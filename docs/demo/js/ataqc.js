//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

/* global Dygraph, List */

var ataqc = (function() {
    return {
        consoleEnabled: false,

        metrics: {},
        experimentList: null,
        tableLists: {},
        activeExperimentIDs: [],
        plots: {},
        tabHelpSeen: {
            'experimentHelp': null,
            'tableHelp': null,
            'plotHelp': null
        },

        listSearchColumns: ['name', 'organism', 'library', 'description'],

        localStorageAvailable: function() {
	        try {
			    x = '__storage_test__';
		        window.localStorage.setItem(x, x);
		        window.localStorage.removeItem(x);
		        return true;
	        }
	        catch(e) {
		        return false;
	        }
        },

        hideHelp: function() {
            for (var visibleHelp of ataqc.querySelectorAll('.visibleHelp')) {
                visibleHelp.classList.remove('visibleHelp');
                if (ataqc.localStorageAvailable()) {
                    window.localStorage.setItem(visibleHelp.id, 'seen');
                }
            }
        },

        showHelp: function(helpID, onlyIfNotSeen=false) {
            if (!onlyIfNotSeen || ataqc.localStorageAvailable() && !window.localStorage.getItem(helpID)) {
                var help = document.getElementById(helpID);
                help.classList.add('visibleHelp');
            }
        },

        log: function(msg) {
            if (ataqc.consoleEnabled) {
                console.log(msg);
            }
        },

        closest: function(startNode, className) {
            var parentNode = startNode.parentNode;
            var result = null;
            while (parentNode.parentNode) {
                if (parentNode.classList.contains(className)) {
                    result = parentNode;
                }
                parentNode = parentNode.parentNode;
            }
            return result;
        },

        formatNumber: function(n) {
            if (isNaN(ataqc.parseInteger(n))) {
                if (isNaN(parseFloat(n))) {
                    return n;
                } else {
                    return n.toPrecision(3);
                }
            } else {
                return n;
            }
        },

        formatIntegerForLocale: window.Intl ?
            new Intl.NumberFormat(
                window.navigator.languages || [window.navigator.language || window.navigator.userLanguage],
                {
                    localeMatcher: 'best fit',
                    maximumFractionDigits: 0
                }
            ).format : ataqc.formatNumber,

        formatNumberForLocale: window.Intl ?
            new Intl.NumberFormat(
                window.navigator.languages || [window.navigator.language || window.navigator.userLanguage],
                {
                    localeMatcher: 'best fit',
                    minimumFractionDigits: 3,
                    maximumFractionDigits: 3
                }
            ).format : ataqc.formatNumber,

        hrefToID: function(href) {
            var id = null;
            if (typeof(href) === 'string' && href.lastIndexOf('#') != -1) {
                id = href.split('#')[1];
            }
            return id;
        },

        parseInteger: function(value) {
            if(/^(\-|\+)?([0-9]+|Infinity)$/.test(value))
                return Number(value);
            return NaN;
        },

        querySelectorAll: function(selectors, elementOrId) {
            var container;
            selectors = typeof(selectors) == 'string' ? [selectors] : selectors;
            if (elementOrId) {
                container = typeof(elementOrId) == 'string' ? document.getElementById(elementOrId) : elementOrId;
            } else {
                container = document;
            }
            var result = selectors.map(function(selector) {return Array.from(container.querySelectorAll(selector));}).reduce(function(p, c, i, a) {return p.concat(c);}, []);
            return result;
        },

        slugify: function(s) {
            return s.replace(/[\s\.]/, '-');
        },

        activateTab: function(tabID) {
            ataqc.hideHelp();
            document.getElementById('help').dataset.helpid = tabID + 'Help';
            var tablist = document.getElementById('tabs');
            for (var t of ataqc.querySelectorAll(['.tab'], tablist)) {
                if (t.dataset.tab === tabID) {
                    t.classList.add('active');
                    t.querySelector('.badge').classList.remove('visible');
                    ataqc.showHelp(tabID + 'Help', true);
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
        },

        clearInput: function(evt) {
            evt.preventDefault();
            if (ataqc.experimentList) {
                var input = evt.currentTarget.previousElementSibling;
                input.value = '';
                ataqc.experimentList.search();
                ataqc.listExperiments('experiments');
            }
        },

        removeActiveExperiment: function(evt) {
            evt.preventDefault();
            var experimentID = evt.currentTarget.dataset.experimentID;
            ataqc.toggleExperiment(experimentID);
            return false;
        },

        attachInputHandlers: function() {
            for (var tab of ataqc.querySelectorAll(['.tab a'])) {
                tab.addEventListener('click', function(evt) {
                    evt.preventDefault();
                    var tabLink = evt.target;
                    if (!tabLink.classList.contains('tabLink')) {
                        tabLink = ataqc.closest(evt.target, 'tabLink');
                    }
                    var id = ataqc.hrefToID(tabLink.href);
                    if (id) {
                        ataqc.activateTab(id);
                        return false;
                    }
                });
            }

            document.getElementById('help').addEventListener('click', function(evt) {
                evt.preventDefault();
                var helpID = evt.target.dataset.helpid;
                ataqc.showHelp(helpID);
            });

            for (var clearButton of ataqc.querySelectorAll(['.inputbox .cleaner'])) {
                clearButton.addEventListener('click', ataqc.clearInput, true);
            }

            for (var helpCloser of ataqc.querySelectorAll(['.help .cleaner'])) {
                helpCloser.addEventListener('click', ataqc.hideHelp, true);
            }

            for (var imager of ataqc.querySelectorAll('.plotImager')) {
                imager.addEventListener('click', ataqc.makePlotImage);
            }

            var addAllButton = document.getElementById('addAllExperiments');
            addAllButton.addEventListener('click', ataqc.addAllExperiments, true);

            var removeAllButton = document.getElementById('removeAllExperiments');
            removeAllButton.addEventListener('click', ataqc.removeAllExperiments, true);

            document.body.addEventListener('keyup', function(evt) {
                if (evt.target.tagName === 'INPUT') {
                    return true;
                }

                switch (evt.keyCode) {
                case 69: ataqc.activateTab('experimentTabBody'); break;
                case 84: ataqc.activateTab('tableTabBody'); break;
                case 80: ataqc.activateTab('plotTabBody'); break;
                }
            });
        },

        installLegendHighlighters: function() {
            var plotContainer = document.getElementById('plots');
            plotContainer.addEventListener('mouseover', function(evt) {
                var plotContainer = ataqc.closest(evt.target, 'plotContainer');
                if (plotContainer) {
                    var plotID = plotContainer.id;
                    var plot = ataqc.plots[plotID];
                    var series = evt.target.dataset.series;
                    if (series) {
                        plot.setSelection(plot.highlightRow || 0, series);
                        for (var li of ataqc.querySelectorAll('li', evt.target.parentNode.parentNode)) {
                            li.classList.remove('highlight');
                        }
                        evt.target.parentNode.classList.add('highlight');
                    }
                }
            });
        },

        listExperiments: function(containerID) {
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

            if (ataqc.experimentList) {
                ataqc.experimentList.clear();
                ataqc.experimentList = null;
            }

            for (experimentID in ataqc.metrics) {
                var experiment_metrics = ataqc.metrics[experimentID];
                experiment_metrics['description_with_url'] = experiment_metrics.url ? '<a target="_blank" href="' + experiment_metrics.url + '">' + experiment_metrics.description + '</a>' : experiment_metrics.description;
                metadata.push(experiment_metrics);
            }

            ataqc.experimentList = new List(containerID, listOptions, metadata);
            var container = document.getElementById(containerID);
            ataqc.experimentList.search(container.querySelector('.search').value, ataqc.listSearchColumns);

            if (ataqc.activeExperimentIDs.length === 0) {
                activeList.innerHTML = '<li class="instruction">Select experiments to compare in the Tables and Plots tabs by clicking on their names on the Experiments tab. Click them again to remove them from the comparison.</li>';
            } else {
                for (experimentID of ataqc.activeExperimentIDs) {
                    var li = document.createElement('li');
                    li.setAttribute('title', ataqc.metrics[experimentID].description);
                    li.classList.add('experiment');
                    li.innerHTML = '<div>' + experimentID + '</div>';

                    var cleaner = document.createElement('button');
                    cleaner.classList.add('cleaner');
                    cleaner.innerHTML = '<i class="fa fa-times-circle"></i>';
                    cleaner.dataset.experimentID = experimentID;
                    cleaner.addEventListener('click', ataqc.removeActiveExperiment, true);

                    li.appendChild(cleaner);

                    activeList.appendChild(li);
                }
            }

            var links = ataqc.querySelectorAll('a.name', containerID);
            for (var a of links) {
                if (ataqc.activeExperimentIDs.indexOf(a.innerHTML) == -1) {
                    a.parentNode.parentNode.classList.remove('active');
                } else {
                    a.parentNode.parentNode.classList.add('active');
                    var cleaner = document.createElement('button');
                    cleaner.classList.add('cleaner');
                    cleaner.innerHTML = '<i class="fa fa-times-circle"></i>';
                    cleaner.dataset.experimentID = a.innerHTML;
                    cleaner.addEventListener('click', ataqc.removeActiveExperiment, true);
                    a.appendChild(cleaner);
                }
            }
        },

        makeLegendFormatter: function(options) {
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

                        html += `<li><div class="legendSeries" style='color: ${series.color};' data-series="${series.label}">${series.labelHTML}</div><div class="legendValue" style='color: ${series.color};'>${series.dashHTML}</div></li>`;
                    }
                    html += '</ul>';
                } else {
                    html = '<div>' + (options.xlabel || '').replace(/{xHTML}/, data.xHTML) + '</div><ul>';
                    var sortedSeries = data.series.sort(function(a, b) {return Number.parseFloat(b.yHTML) - Number.parseFloat(a.yHTML);})
                    for (i = 0; i < sortedSeries.length; i++) {
                        series = sortedSeries[i];
                        if (!series.isVisible) continue;
                        var cls = series.isHighlighted ? ' class="highlight"' : '';
                        html += `<li${cls}><div class="legendSeries" style='color: ${series.color};' data-series="${series.label}">${series.labelHTML}</div><div class="legendValue">${series.yHTML}</div></li>`;
                    }
                    html += '</ul>';
                }
                return html;
            };
        },

        plotFragmentLength: function(containerID) {
            var container = document.getElementById(containerID);
            var plotElement = container.querySelector('.plot');
            var legendElement = container.querySelector('.legend');

            var options = {
                axes: {
                    y: {
                        valueFormatter: ataqc.formatNumber
                    }
                },
                axisLabelWidth: 80,
                hideOverlayOnMouseOut: false,
                highlightCallback:  function(event, x, points, row, seriesName) {
                    ataqc.plots.fragmentLength.highlightRow = row;
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
                legendFormatter: ataqc.makeLegendFormatter({xlabel: 'Fragment length: {xHTML}'}),
                panEdgeFraction: 0.1,
                xAxisHeight: 40,
                xlabel: 'Fragment Length',
                ylabel: 'Fraction of all reads'
            };

            var data = [];
            for (var i = 0; i < 1000; i++) {
                data[i] = [i];
            }

            for (var e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                var experimentID = ataqc.activeExperimentIDs[e];
                options.labels.push(experimentID);
                var experiment = ataqc.metrics[experimentID];
                for (var v = 0; v < 1000; v++) {
                    data[v].push(experiment.fragment_length_counts[v][2]);
                }
            }

            if (ataqc.plots.fragmentLength) {
                ataqc.plots.fragmentLength.destroy();
            }
            ataqc.plots.fragmentLength = new Dygraph(plotElement, data, options);
        },

        plotMapq: function(containerID) {
            var container = document.getElementById(containerID);
            var plot = container.querySelector('.plot');
            var legend = container.querySelector('.legend');

            var options = {
                axes: {
                    y: {
                        valueFormatter: ataqc.formatNumber
                    }
                },
                axisLabelWidth: 80,
                hideOverlayOnMouseOut: false,
                highlightCallback:  function(event, x, points, row, seriesName) {
                    ataqc.plots.mapq.highlightRow = row;
                },
                highlightSeriesOpts: {
                    strokeWidth: 3,
                    strokeBorderWidth: 1,
                    highlightCircleSize: 3
                },
                labels: ['x'],
                labelsDiv: legend,
                legend: 'always',
                legendFormatter: ataqc.makeLegendFormatter({xlabel: 'Mapping quality: {xHTML}'}),
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
            for (e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                experimentID = ataqc.activeExperimentIDs[e];
                options.labels.push(experimentID);
                mapqMap[experimentID] = [];
                var experiment = ataqc.metrics[experimentID];
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
                for (e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                    experimentID = ataqc.activeExperimentIDs[e];
                    data[i].push(mapqMap[experimentID][i]);
                }
            }

            if (ataqc.plots.mapq) {
                ataqc.plots.mapq.destroy();
            }
            ataqc.plots.mapq = new Dygraph(plot, data, options);
        },

        plotPeakReadCounts: function(containerID) {
            var container = document.getElementById(containerID);
            var plot = container.querySelector('.plot');
            var legend = container.querySelector('.legend');

            var options = {
                axes: {
                    y: {
                        valueFormatter: ataqc.formatNumber
                    }
                },
                axisLabelWidth: 80,
                hideOverlayOnMouseOut: false,
                highlightCallback:  function(event, x, points, row, seriesName) {
                    ataqc.plots.peakReadCounts.highlightRow = row;
                },
                highlightSeriesOpts: {
                    strokeWidth: 3,
                    strokeBorderWidth: 1,
                    highlightCircleSize: 3
                },
                labels: ['x'],
                labelsDiv: legend,
                legend: 'always',
                legendFormatter: ataqc.makeLegendFormatter({xlabel: 'Cumulative fraction of high quality autosomal alignments at {xHTML} percent of peaks: '}),
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

            for (e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                experimentID = ataqc.activeExperimentIDs[e];
                options.labels.push(experimentID);

                peakMap[experimentID] = [];

                var experiment = ataqc.metrics[experimentID];
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
                    for (e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                        experimentID = ataqc.activeExperimentIDs[e];
                        data[i].push(peakMap[experimentID][i]);
                    }
                }

                if (ataqc.plots.peakReadCounts) {
                    ataqc.plots.peakReadCounts.destroy();
                }
                ataqc.plots.peakReadCounts = new Dygraph(plot, data, options);
            } else {
                plot.innerHTML = '<p class="warning">The selected experiment' + (ataqc.activeExperimentIDs.length == 1 ? ' has' : 's have') + ' no peaks.</p>';
            }
        },

        plotPeakTerritory: function(containerID) {
            var container = document.getElementById(containerID);
            var plot = container.querySelector('.plot');
            var legend = container.querySelector('.legend');

            var options = {
                axes: {
                    y: {
                        valueFormatter: ataqc.formatNumber
                    }
                },
                axisLabelWidth: 80,
                hideOverlayOnMouseOut: false,
                highlightCallback:  function(event, x, points, row, seriesName) {
                    ataqc.plots.peakTerritory.highlightRow = row;
                },
                highlightSeriesOpts: {
                    strokeWidth: 3,
                    strokeBorderWidth: 1,
                    highlightCircleSize: 3
                },
                labels: ['x'],
                labelsDiv: legend,
                legend: 'always',
                legendFormatter: ataqc.makeLegendFormatter({xlabel: 'Cumulative fraction of territory at {xHTML} percent of peaks: '}),
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

            for (e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                experimentID = ataqc.activeExperimentIDs[e];
                options.labels.push(experimentID);

                peakMap[experimentID] = [];

                var experiment = ataqc.metrics[experimentID];
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
                    for (e = 0; e < ataqc.activeExperimentIDs.length; e++) {
                        experimentID = ataqc.activeExperimentIDs[e];
                        data[i].push(peakMap[experimentID][i]);
                    }
                }
                if (ataqc.plots.peakTerritory) {
                    ataqc.plots.peakTerritory.destroy();
                }
                ataqc.plots.peakTerritory = new Dygraph(plot, data, options);
            } else {
                plot.innerHTML = '<p class="warning">The selected experiment' + (ataqc.activeExperimentIDs.length == 1 ? ' has' : 's have') + ' no peaks.</p>';
            }
        },

        populateTables: function() {
            for (var table of ataqc.querySelectorAll('table.data')) {
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
                for (var headerCell of ataqc.querySelectorAll(['th[data-metric]'], table)) {
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
                for (experimentID of ataqc.activeExperimentIDs) {
                    experiment = ataqc.metrics[experimentID];

                    experimentValues = {
                        title: experiment.description
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
                    row.dataset['experiment'] = ataqc.slugify(experimentValues.name);
                    for (key of keys) {
                        value = experimentValues[key];
                        cell = row.insertCell();
                        cell.classList.add(key);
                        cell.dataset['experiment'] = ataqc.slugify(experimentValues.name);
                        cell.setAttribute('title', experimentValues.title);
                        if (typeof(value) == 'number') {
                            if (value == ranges[key].max && value != ranges[key].min) {
                                cell.classList.add('max');
                            }

                            if (value == ranges[key].min && value != ranges[key].max) {
                                cell.classList.add('min');
                            }

                            if (types[key] === 'integer') {
                                value = ataqc.formatIntegerForLocale(value);
                            } else {
                                value = ataqc.formatNumberForLocale(value);
                            }
                        }
                        cell.innerHTML = value;
                    }
                }
                tbody.addEventListener('mouseover', ataqc.highlightExperimentRows, true);
                tbody.addEventListener('mouseout', ataqc.highlightExperimentRows, true);

                if (table.id in Object.keys(ataqc.tableLists)) {
                    ataqc.tableLists[table.id].clear();
                    ataqc.tableLists[table.id] = null;
                }

                if (values.length) {
                    var options = {
                        valueNames: keys
                    };
                    ataqc.tableLists[table.id] = new List(table.id, options);
                }
            }
            if (ataqc.activeExperimentIDs.length) {
                document.getElementById('tables').style.display = 'block';
            } else {
                document.getElementById('tables').style.display = 'none';
            }
        },

        highlightExperimentRows: function(evt) {
            for (var tr of ataqc.querySelectorAll(['tr[data-experiment=' + evt.target.dataset.experiment + ']'])) {
                tr.classList.toggle('highlight');
            }
        },

        loadExperiments: function(containerID) {
            var container = document.getElementById(containerID);
            var tbody = container.querySelector('.list');
            tbody.addEventListener('click', function(evt) {
                if (evt.target.classList.contains('name') && evt.target.href) {
                    evt.preventDefault();
                    var experimentID = evt.target.innerText;
                    ataqc.toggleExperiment(experimentID);
                }
                return false;
            }, true);

            ataqc.activeExperimentIDs = [];
            ataqc.listExperiments(containerID);
        },

        populatePlots: function() {
            if (ataqc.activeExperimentIDs.length) {
                document.getElementById('plots').style.display = 'block';
            } else {
                document.getElementById('plots').style.display = 'none';
            }
            ataqc.plotFragmentLength('fragmentLength');
            ataqc.plotMapq('mapq');
            ataqc.plotPeakReadCounts('peakReadCounts');
            ataqc.plotPeakTerritory('peakTerritory');
        },

        update: function() {
            ataqc.activeExperimentIDs.sort();
            ataqc.listExperiments('experiments');
            ataqc.populateTables();
            ataqc.populatePlots();
            ataqc.installLegendHighlighters();
            if (ataqc.activeExperimentIDs.length > 0) {
                document.getElementById('removeAllExperiments').style.display = 'inline-block';
                for (inactive_tab of document.querySelectorAll('.tab:not(.active) a')) {
                    inactive_tab.querySelector('.badge').classList.add('visible');
                    inactive_tab.classList.add('glow');
                }
                window.setTimeout(function() {
                    for (inactive_tab of document.querySelectorAll('.tab:not(.active) a')) {
                        inactive_tab.classList.remove('glow');
                    }
                }, 300);
            } else {
                for (badge of document.querySelectorAll('.badge')) {
                    badge.classList.remove('visible');
                }
            }
        },

        addAllExperiments: function(evt) {
            for (var experiment of ataqc.experimentList.visibleItems) {
                var experimentID = experiment.values().name;
                var activeIndex = ataqc.activeExperimentIDs.indexOf(experimentID);
                if (activeIndex === -1) {
                    ataqc.activeExperimentIDs.push(experimentID);
                }
            }
            window.setTimeout(function() {
                ataqc.update();
            }, 100);
        },

        removeAllExperiments: function(evt) {
            for (var experiment of ataqc.experimentList.visibleItems) {
                var experimentID = experiment.values().name;
                var activeIndex = ataqc.activeExperimentIDs.indexOf(experimentID);
                if (activeIndex >= 0) {
                    ataqc.activeExperimentIDs.splice(activeIndex, 1);
                }
            }
            window.setTimeout(function() {
                ataqc.update();
            }, 100);
        },

        toggleExperiment: function(experimentID) {
            window.setTimeout(function() {
                var activeIndex = ataqc.activeExperimentIDs.indexOf(experimentID);
                if (activeIndex >= 0) {
                    ataqc.activeExperimentIDs.splice(activeIndex, 1);
                    ataqc.update();
                } else if (experimentID in ataqc.metrics) {
                    ataqc.activeExperimentIDs.push(experimentID);
                    ataqc.update();
                }
            }, 100);
        },

        initialize: function(containerID) {
            ataqc.attachInputHandlers();
            ataqc.loadExperiments(containerID);
            window.addEventListener('resize', ataqc.populatePlots);
            ataqc.showHelp('experimentTabBodyHelp', true);
        }
    }
})();
