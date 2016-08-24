//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

/* global Dygraph, List */

var ataqc = (function() {
    var consoleEnabled = false;

    var metrics = {};
    var experimentList = null;
    var tableLists = {};
    var activeExperimentIDs = [];
    var plots = {};

    var listSearchColumns = ['name', 'organism', 'library', 'description'];

    function localStorageAvailable() {
        try {
            x = '__storage_test__';
            window.localStorage.setItem(x, x);
            window.localStorage.removeItem(x);
            return true;
        }
        catch(e) {
            return false;
        }
    }

    function hideHelp() {
        for (var visibleHelp of querySelectorAll('.visibleHelp')) {
            visibleHelp.classList.remove('visibleHelp');
            if (localStorageAvailable()) {
                window.localStorage.setItem(visibleHelp.id, 'seen');
            }
        }
    }

    function showHelp(helpID, onlyIfNotSeen) {
        if (!onlyIfNotSeen || localStorageAvailable() && !window.localStorage.getItem(helpID)) {
            var help = document.getElementById(helpID);
            help.classList.add('visibleHelp');
        }
    }

    function log(msg) {
        if (consoleEnabled) {
            console.log(msg);
        }
    }

    function closest(startNode, className) {
        var parentNode = startNode.parentNode;
        var result = null;
        while (parentNode.parentNode) {
            if (parentNode.classList.contains(className)) {
                result = parentNode;
            }
            parentNode = parentNode.parentNode;
        }
        return result;
    }

    function escapeAttribute(text) {
        return text.replace(/\\?"/g, "'");
    }

    function formatNumber(n) {
        if (isNaN(parseInteger(n))) {
            if (isNaN(parseFloat(n))) {
                return n;
            } else {
                return n.toPrecision(3);
            }
        } else {
            return n;
        }
    }

    var formatIntegerForLocale = window.Intl ?
        new Intl.NumberFormat(
            window.navigator.languages || [window.navigator.language || window.navigator.userLanguage],
            {
                localeMatcher: 'best fit',
                maximumFractionDigits: 0
            }
        ).format : formatNumber;

    var formatNumberForLocale = window.Intl ?
        new Intl.NumberFormat(
            window.navigator.languages || [window.navigator.language || window.navigator.userLanguage],
            {
                localeMatcher: 'best fit',
                minimumFractionDigits: 3,
                maximumFractionDigits: 3
            }
        ).format : formatNumber;

    function hrefToID(href) {
        var id = null;
        if (typeof(href) === 'string' && href.lastIndexOf('#') != -1) {
            id = href.split('#')[1];
        }
        return id;
    }

    function parseInteger(value) {
        if(/^(\-|\+)?([0-9]+|Infinity)$/.test(value))
            return Number(value);
        return NaN;
    }

    function querySelectorAll(selectors, elementOrId) {
        var container;
        selectors = typeof(selectors) == 'string' ? [selectors] : selectors;
        if (elementOrId) {
            container = typeof(elementOrId) == 'string' ? document.getElementById(elementOrId) : elementOrId;
        } else {
            container = document;
        }
        var result = selectors.map(function(selector) {return Array.from(container.querySelectorAll(selector));}).reduce(function(p, c, i, a) {return p.concat(c);}, []);
        return result;
    }

    function slugify(s) {
        return s.replace(/[\s\.]/, '-');
    }

    function activateTab(tabID) {
        hideHelp();
        document.getElementById('help').dataset.helpid = tabID + 'Help';
        var tablist = document.getElementById('tabs');
        for (var t of querySelectorAll(['.tab'], tablist)) {
            if (t.dataset.tab === tabID) {
                t.classList.add('active');
                t.querySelector('.badge').classList.remove('visible');
                showHelp(tabID + 'Help', true);
            } else {
                t.classList.remove('active');
            }

            var tab = document.getElementById(t.dataset.tab);
            if (tab.id === tabID) {
                tab.classList.add('active');
            } else {
                tab.classList.remove('active');
            }
        }
    }

    function clearInput(evt) {
        evt.preventDefault();
        if (experimentList) {
            var input = evt.currentTarget.previousElementSibling;
            input.value = '';
            experimentList.search();
            listExperiments('experiments');
        }
    }

    function removeActiveExperiment(evt) {
        evt.preventDefault();
        var experimentID = evt.currentTarget.dataset.experimentID;
        toggleExperiment(experimentID);
        return false;
    }

    function addEventListeners() {
        window.addEventListener('resize', populatePlots);

        for (var tab of querySelectorAll(['.tab a'])) {
            tab.addEventListener('click', function(evt) {
                evt.preventDefault();
                var tabLink = evt.target;
                if (!tabLink.classList.contains('tabLink')) {
                    tabLink = closest(evt.target, 'tabLink');
                }
                var id = hrefToID(tabLink.href);
                if (id) {
                    activateTab(id);
                    return false;
                }
            });
        }

        document.getElementById('help').addEventListener('click', function(evt) {
            evt.preventDefault();
            var helpID = evt.target.dataset.helpid;
            showHelp(helpID);
        });

        for (var clearButton of querySelectorAll(['.inputbox .cleaner'])) {
            clearButton.addEventListener('click', clearInput, true);
        }

        for (var helpCloser of querySelectorAll(['.help .cleaner'])) {
            helpCloser.addEventListener('click', hideHelp, true);
        }

        for (var imager of querySelectorAll('.plotImager')) {
            imager.addEventListener('click', makePlotImage);
        }

        var addAllButton = document.getElementById('addAllExperiments');
        addAllButton.addEventListener('click', addAllExperiments, true);

        var removeAllButton = document.getElementById('removeAllExperiments');
        removeAllButton.addEventListener('click', removeAllExperiments, true);

        document.body.addEventListener('keyup', function(evt) {
            if (evt.target.tagName === 'INPUT') {
                return true;
            }

            switch (evt.keyCode) {
            case 69: activateTab('experimentTabBody'); break;
            case 84: activateTab('tableTabBody'); break;
            case 80: activateTab('plotTabBody'); break;
            }
        });
    }

    function installLegendHighlighters() {
        for (var legend of querySelectorAll('.legend')) {
            legend.addEventListener('mouseover', function(evt) {
                evt.preventDefault();
                var plotContainer = closest(evt.target, 'plotContainer');
                if (plotContainer) {
                    var plotID = plotContainer.id;
                    var plot = plots[plotID];
                    var li = evt.target.classList.contains('legendItem') ? evt.target : closest(evt.target, 'legendItem');
                    if (li) {
                        var series = li.dataset.series;
                        plot.setSelection(plot.highlightRow || 0, series);
                        for (var sib of querySelectorAll('li', li.parentNode)) {
                            sib.classList.remove('highlight');
                        }
                        li.classList.add('highlight');
                    }
                }
            });
        }
    }

    function listExperiments(containerID) {
        var experimentID;
        var metadata = [];

        var listOptions = {
            item: '<tr><td class=""><a href="#" class="name"></a></td><td class="organism"></td><td class="library"></td><td class="description_with_url"></td><td><a href="#" class="original_url" download>Download</a></td></tr>',
            valueNames: [
                'name',
                'organism',
                'library',
                'description_with_url',
                {name: 'original_url', attr: 'href'},
                {name: 'original_url', attr: 'download'}
            ]
        };

        var activeList = document.getElementById('active_experiments');
        activeList.innerHTML = '';

        if (experimentList) {
            experimentList.clear();
            experimentList = null;
        }

        for (experimentID in metrics) {
            var experiment_metrics = metrics[experimentID];
            experiment_metrics['description_with_url'] = experiment_metrics.url ? '<a target="_blank" href="' + experiment_metrics.url + '">' + experiment_metrics.description + '</a>' : experiment_metrics.description;
            metadata.push(experiment_metrics);
        }

        experimentList = new List(containerID, listOptions, metadata);
        var container = document.getElementById(containerID);
        experimentList.search(container.querySelector('.search').value, listSearchColumns);

        if (activeExperimentIDs.length === 0) {
            activeList.innerHTML = '<li class="instruction">Select experiments to compare in the Tables and Plots tabs by clicking on their names on the Experiments tab. Click them again to remove them from the comparison.</li>';
        } else {
            for (experimentID of activeExperimentIDs) {
                var li = document.createElement('li');
                li.dataset.hovertext = escapeAttribute(metrics[experimentID].description);
                li.classList.add('experiment');
                li.innerHTML = '<div>' + experimentID + '</div>';

                var cleaner = document.createElement('button');
                cleaner.classList.add('cleaner');
                cleaner.innerHTML = '<i class="fa fa-times-circle"></i>';
                cleaner.dataset.experimentID = experimentID;
                cleaner.addEventListener('click', removeActiveExperiment, true);

                li.appendChild(cleaner);

                activeList.appendChild(li);
            }
        }

        var links = querySelectorAll('a.name', containerID);
        for (var a of links) {
            if (activeExperimentIDs.indexOf(a.innerHTML) == -1) {
                a.parentNode.parentNode.classList.remove('active');
            } else {
                a.parentNode.parentNode.classList.add('active');
                var cleaner = document.createElement('button');
                cleaner.classList.add('cleaner');
                cleaner.innerHTML = '<i class="fa fa-times-circle"></i>';
                cleaner.dataset.experimentID = a.innerHTML;
                cleaner.addEventListener('click', removeActiveExperiment, true);
                a.appendChild(cleaner);
            }
        }
    }

    function makeLegendFormatter(options) {
        return function(data) {
            var g = data.dygraph;
            var html;
            var i;
            var series;
            if (typeof(data.x) === 'undefined') {
                if (g.getOption('legend') != 'always') {
                    return '';
                }

                html = '<div>Experiments</div><ul>';
                for (i = 0; i < data.series.length; i++) {
                    series = data.series[i];
                    if (!series.isVisible) continue;

                    var experimentMetrics = metrics[series.labelHTML];
                    var hovertext = escapeAttribute(experimentMetrics.description);

                    html += `<li class="legendItem" data-series="${series.label}" data-hovertext="${hovertext}"><div class="legendSeries" style='color: ${series.color};'>${series.labelHTML}</div><div class="legendValue" style='color: ${series.color};'>${series.dashHTML}</div></li>`;
                }
                html += '</ul>';
            } else {
                html = '<div>' + (options.xlabel || '').replace(/{xHTML}/, data.xHTML) + '</div><ul>';
                var sortedSeries = data.series.sort(function(a, b) {return Number.parseFloat(b.yHTML) - Number.parseFloat(a.yHTML);})
                for (i = 0; i < sortedSeries.length; i++) {
                    series = sortedSeries[i];
                    if (!series.isVisible) continue;

                    var experimentMetrics = metrics[series.labelHTML];
                    var hovertext = escapeAttribute(experimentMetrics.description);

                    var highlightClass = series.isHighlighted ? 'highlight' : '';
                    html += `<li class="legendItem ${highlightClass}" data-series="${series.label}" data-hovertext="${hovertext}"><div class="legendSeries" style='color: ${series.color};'>${series.labelHTML}</div><div class="legendValue">${series.yHTML}</div></li>`;
                }
                html += '</ul>';
            }
            return html;
        };
    }

    function plotFragmentLength(containerID) {
        var container = document.getElementById(containerID);
        var plotElement = container.querySelector('.plot');
        var legendElement = container.querySelector('.legend');

        var options = {
            axes: {
                y: {
                    valueFormatter: formatNumber
                }
            },
            axisLabelWidth: 80,
            hideOverlayOnMouseOut: false,
            highlightCallback:  function(event, x, points, row, seriesName) {
                plots.fragmentLength.highlightRow = row;
            },
            highlightSeriesOpts: {
                strokeWidth: 3,
                strokeBorderWidth: 1,
                highlightCircleSize: 3,
                highlightSeriesBackgroundAlpha: 0.1
            },
            labels: ['x'],
            labelsDiv: legendElement,
            legend: 'always',
            legendFormatter: makeLegendFormatter({xlabel: 'Fragment length: {xHTML}'}),
            panEdgeFraction: 0.1,
            xAxisHeight: 40,
            xlabel: 'Fragment Length',
            ylabel: 'Fraction of all reads'
        };

        var data = [];
        for (var i = 0; i < 1000; i++) {
            data[i] = [i];
        }

        for (var e = 0; e < activeExperimentIDs.length; e++) {
            var experimentID = activeExperimentIDs[e];
            options.labels.push(experimentID);
            var experiment = metrics[experimentID];
            for (var v = 0; v < 1000; v++) {
                data[v].push(experiment.fragment_length_counts[v][2]);
            }
        }

        if (plots.fragmentLength) {
            plots.fragmentLength.destroy();
        }
        plots.fragmentLength = new Dygraph(plotElement, data, options);
    }

    function plotMapq(containerID) {
        var container = document.getElementById(containerID);
        var plot = container.querySelector('.plot');
        var legend = container.querySelector('.legend');

        var options = {
            axes: {
                y: {
                    valueFormatter: formatNumber
                }
            },
            axisLabelWidth: 80,
            hideOverlayOnMouseOut: false,
            highlightCallback:  function(event, x, points, row, seriesName) {
                plots.mapq.highlightRow = row;
            },
            highlightSeriesOpts: {
                strokeWidth: 3,
                strokeBorderWidth: 1,
                highlightCircleSize: 3
            },
            labels: ['x'],
            labelsDiv: legend,
            legend: 'always',
            legendFormatter: makeLegendFormatter({xlabel: 'Mapping quality: {xHTML}'}),
            panEdgeFraction: 0.1,
            xAxisHeight: 40,
            xlabel: 'Mapping quality',
            ylabel: 'Fraction of all reads'
        };

        var data = [];

        var maxMAPQ = 0;
        var mapqMap = {};
        var e;
        var experimentID;
        for (e = 0; e < activeExperimentIDs.length; e++) {
            experimentID = activeExperimentIDs[e];
            options.labels.push(experimentID);
            mapqMap[experimentID] = [];
            var experiment = metrics[experimentID];
            var totalReads = experiment.total_reads;
            for (var mqc of experiment.mapq_counts) {
                if (maxMAPQ < mqc[0]) {
                    maxMAPQ = mqc[0];
                }

                var normed = mqc[1] / totalReads;
                mapqMap[experimentID].push(normed);
            }
        }

        for (var i = 0; i <= maxMAPQ; i++) {
            data[i] = [i];
            for (e = 0; e < activeExperimentIDs.length; e++) {
                experimentID = activeExperimentIDs[e];
                data[i].push(mapqMap[experimentID][i]);
            }
        }

        if (plots.mapq) {
            plots.mapq.destroy();
        }
        plots.mapq = new Dygraph(plot, data, options);
    }

    function plotPeakReadCounts(containerID) {
        var container = document.getElementById(containerID);
        var plot = container.querySelector('.plot');
        var legend = container.querySelector('.legend');

        var options = {
            axes: {
                y: {
                    valueFormatter: formatNumber
                }
            },
            axisLabelWidth: 80,
            hideOverlayOnMouseOut: false,
            highlightCallback:  function(event, x, points, row, seriesName) {
                plots.peakReadCounts.highlightRow = row;
            },
            highlightSeriesOpts: {
                strokeWidth: 3,
                strokeBorderWidth: 1,
                highlightCircleSize: 3
            },
            labels: ['x'],
            labelsDiv: legend,
            legend: 'always',
            legendFormatter: makeLegendFormatter({xlabel: 'Cumulative fraction of high quality autosomal alignments at {xHTML} percent of peaks: '}),
            panEdgeFraction: 0.1,
            xAxisHeight: 40,
            xlabel: 'Peak Percentile',
            ylabel: 'Cumulative fraction of alignments'
        };

        var data = [];

        var percentilesSeen = false;
        var peakMap = {};
        var i;
        var e;
        var experimentID;

        for (e = 0; e < activeExperimentIDs.length; e++) {
            experimentID = activeExperimentIDs[e];
            options.labels.push(experimentID);

            peakMap[experimentID] = [];

            var experiment = metrics[experimentID];
            var percentiles = experiment.peak_percentiles.cumulative_fraction_of_hqaa;
            var percentileCount = percentiles ? percentiles.length : 0;
            percentilesSeen = percentilesSeen || (percentileCount > 0);

            if (percentileCount > 0) {
                for (var peakIndex = 0; peakIndex < 100; peakIndex++) {
                    peakMap[experimentID].push(percentiles[peakIndex]);
                }
            }
        }

        if (percentilesSeen) {
            for (i = 0; i < 100; i++) {
                data[i] = [i + 1];
                for (e = 0; e < activeExperimentIDs.length; e++) {
                    experimentID = activeExperimentIDs[e];
                    data[i].push(peakMap[experimentID][i]);
                }
            }

            if (plots.peakReadCounts) {
                plots.peakReadCounts.destroy();
            }
            plots.peakReadCounts = new Dygraph(plot, data, options);
        } else {
            plot.innerHTML = '<p class="warning">The selected experiment' + (activeExperimentIDs.length == 1 ? ' has' : 's have') + ' no peaks.</p>';
        }
    }

    function plotPeakTerritory(containerID) {
        var container = document.getElementById(containerID);
        var plot = container.querySelector('.plot');
        var legend = container.querySelector('.legend');

        var options = {
            axes: {
                y: {
                    valueFormatter: formatNumber
                }
            },
            axisLabelWidth: 80,
            hideOverlayOnMouseOut: false,
            highlightCallback:  function(event, x, points, row, seriesName) {
                plots.peakTerritory.highlightRow = row;
            },
            highlightSeriesOpts: {
                strokeWidth: 3,
                strokeBorderWidth: 1,
                highlightCircleSize: 3
            },
            labels: ['x'],
            labelsDiv: legend,
            legend: 'always',
            legendFormatter: makeLegendFormatter({xlabel: 'Cumulative fraction of territory at {xHTML} percent of peaks: '}),
            panEdgeFraction: 0.1,
            xAxisHeight: 40,
            xlabel: 'Peak Percentile',
            ylabel: 'Cumulative fraction of peak territory'
        };

        var data = [];

        var percentilesSeen = false;
        var peakMap = {};
        var e;
        var experimentID;

        for (e = 0; e < activeExperimentIDs.length; e++) {
            experimentID = activeExperimentIDs[e];
            options.labels.push(experimentID);

            peakMap[experimentID] = [];

            var experiment = metrics[experimentID];
            var percentiles = experiment.peak_percentiles.cumulative_fraction_of_territory;
            var percentileCount = percentiles ? percentiles.length : 0;
            percentilesSeen = percentilesSeen || (percentileCount > 0);

            if (percentileCount) {
                for (var peakIndex = 0; peakIndex < 101; peakIndex++) {
                    peakMap[experimentID].push(percentiles[peakIndex]);
                }
            }
        }

        if (percentilesSeen) {
            for (var i = 0; i < 101; i++) {
                data[i] = [i + 1];
                for (e = 0; e < activeExperimentIDs.length; e++) {
                    experimentID = activeExperimentIDs[e];
                    data[i].push(peakMap[experimentID][i]);
                }
            }
            if (plots.peakTerritory) {
                plots.peakTerritory.destroy();
            }
            plots.peakTerritory = new Dygraph(plot, data, options);
        } else {
            plot.innerHTML = '<p class="warning">The selected experiment' + (activeExperimentIDs.length == 1 ? ' has' : 's have') + ' no peaks.</p>';
        }
    }

    function populateTables() {
        for (var table of querySelectorAll('table.data')) {
            var keys = [];
            var key;
            var value;
            var cell;
            var experimentID;
            var experiment;
            var experimentValues;
            var denominators = {};
            var percentages = {};
            var types = {};
            for (var headerCell of querySelectorAll(['th[data-metric]'], table)) {
                key = headerCell.dataset.metric;
                keys.push(key);
                if ('metrictype' in headerCell.dataset) {
                    types[key] = headerCell.dataset.metrictype;
                } else {
                    types[key] = 'number';
                }

                if ('denominator' in headerCell.dataset) {
                    denominators[key] = headerCell.dataset.denominator;
                } else if ('percentage' in headerCell.dataset) {
                    percentages[key] = headerCell.dataset.percentage;
                }
            }

            var values = [];
            var ranges = {};
            for (experimentID of activeExperimentIDs) {
                experiment = metrics[experimentID];

                experimentValues = {
                    description: experiment.description
                };

                for (key of keys) {
                    value = experiment[key];

                    if (typeof(value) == 'number') {
                        if (denominators[key]) {
                            value /= experiment[denominators[key]] || 1;
                        } else if (percentages[key]) {
                            value /= (experiment[percentages[key]] / 100.0);
                        }
                    }

                    experimentValues[key] = value;

                    if (key in ranges) {
                        if (value > ranges[key].max) {
                            ranges[key].max = value;
                        } else if (value < ranges[key].min) {
                            ranges[key].min = value;
                        }
                    } else {
                        ranges[key] = {
                            max: value,
                            min: value
                        };
                    }
                }
                values.push(experimentValues);
            }
            var tbody = table.querySelector('tbody');
            tbody.innerHTML = '';
            for (experimentValues of values) {
                var row = tbody.insertRow();
                row.dataset['experiment'] = slugify(experimentValues.name);
                for (key of keys) {
                    value = experimentValues[key];
                    cell = row.insertCell();
                    cell.classList.add(key);
                    cell.dataset.experiment = slugify(experimentValues.name);
                    cell.dataset.hovertext = escapeAttribute(experimentValues.description);
                    if (typeof(value) == 'number') {
                        if (value == ranges[key].max && value != ranges[key].min) {
                            cell.classList.add('max');
                        }

                        if (value == ranges[key].min && value != ranges[key].max) {
                            cell.classList.add('min');
                        }

                        if (types[key] === 'integer') {
                            value = formatIntegerForLocale(value);
                        } else {
                            value = formatNumberForLocale(value);
                        }
                    }
                    cell.innerHTML = value;
                }
            }
            tbody.addEventListener('mouseover', highlightExperimentRows, true);
            tbody.addEventListener('mouseout', highlightExperimentRows, true);

            if (table.id in Object.keys(tableLists)) {
                tableLists[table.id].clear();
                tableLists[table.id] = null;
            }

            if (values.length) {
                var options = {
                    valueNames: keys
                };
                tableLists[table.id] = new List(table.id, options);
            }
        }
        if (activeExperimentIDs.length) {
            document.getElementById('tables').style.visibility = 'visible';
        } else {
            document.getElementById('tables').style.visibility = 'hidden';
        }
    }

    function highlightExperimentRows(evt) {
        for (var tr of querySelectorAll(['tr[data-experiment=' + evt.target.dataset.experiment + ']'])) {
            tr.classList.toggle('highlight');
        }
    }

    function loadExperiments(containerID) {
        var container = document.getElementById(containerID);
        var tbody = container.querySelector('.list');
        tbody.addEventListener('click', function(evt) {
            if (evt.target.classList.contains('name') && evt.target.href) {
                evt.preventDefault();
                var experimentID = evt.target.innerText;
                toggleExperiment(experimentID);
            }
            return false;
        }, true);

        activeExperimentIDs = [];
        listExperiments(containerID);
    }

    function populatePlots() {
        if (activeExperimentIDs.length) {
            document.getElementById('plots').style.visibility = 'visible';
        } else {
            document.getElementById('plots').style.visibility = 'hidden';
        }
        plotFragmentLength('fragmentLength');
        plotMapq('mapq');
        plotPeakReadCounts('peakReadCounts');
        plotPeakTerritory('peakTerritory');
    }

    function update() {
        activeExperimentIDs.sort();
        listExperiments('experiments');
        populateTables();
        populatePlots();
        installLegendHighlighters();
        if (activeExperimentIDs.length > 0) {
            document.getElementById('removeAllExperiments').style.display = 'inline-block';
            for (inactive_tab of querySelectorAll('.tab:not(.active) a')) {
                inactive_tab.querySelector('.badge').classList.add('visible');
                inactive_tab.classList.add('glow');
            }
            window.setTimeout(function() {
                for (inactive_tab of querySelectorAll('.tab:not(.active) a')) {
                    inactive_tab.classList.remove('glow');
                }
            }, 300);
        } else {
            for (badge of querySelectorAll('.badge')) {
                badge.classList.remove('visible');
            }
        }
    }

    function addAllExperiments(evt) {
        for (var experiment of experimentList.visibleItems) {
            var experimentID = experiment.values().name;
            var activeIndex = activeExperimentIDs.indexOf(experimentID);
            if (activeIndex === -1) {
                activeExperimentIDs.push(experimentID);
            }
        }
        window.setTimeout(function() {
            update();
        }, 100);
    }

    function removeAllExperiments(evt) {
        for (var experiment of experimentList.visibleItems) {
            var experimentID = experiment.values().name;
            var activeIndex = activeExperimentIDs.indexOf(experimentID);
            if (activeIndex >= 0) {
                activeExperimentIDs.splice(activeIndex, 1);
            }
        }
        window.setTimeout(function() {
            update();
        }, 100);
    }

    function toggleExperiment(experimentID) {
        window.setTimeout(function() {
            var activeIndex = activeExperimentIDs.indexOf(experimentID);
            if (activeIndex >= 0) {
                activeExperimentIDs.splice(activeIndex, 1);
                update();
            } else if (experimentID in metrics) {
                activeExperimentIDs.push(experimentID);
                update();
            }
        }, 100);
    }

    function initialize(containerID) {
        addEventListeners();
        loadExperiments(containerID);
        showHelp('experimentTabBodyHelp', true);
    }

    function setMetrics(newMetrics) {
        metrics = newMetrics;
    }

    return {
        setMetrics: setMetrics,
        initialize: initialize
    }
})();
