//
// Copyright 2015 Stephen Parker
//
// Licensed under Version 3 of the GPL or any later version
//

/* global d3, $ */

'use strict;'

let ataqv = (function() {
    let consoleEnabled = true;

    let configuration = {};

    let plotPalette = [
        // blue, green, purple, yellow-or-brown
        "#6baed6",
        "#74c476",
        "#9e9ac8",
        "#fe9929",

        "#2171b5",
        "#238b45",
        "#6a51a3",
        "#cc4c02",

        "#4292c6",
        "#41ab5d",
        "#807dba",
        "#ec7014",

        "#9ecae1",
        "#a1d99b",
        "#bcbddc",
        "#fec44f",

        "#c6dbef",
        "#c7e9c0",
        "#dadaeb",
        "#fee391",

        "#deebf7",
        "#e5f5e0",
        "#efedf5",
        "#fff7bc",

        "#08306b",
        "#00441b",
        "#3f007d",
        "#662506",

        "#08519c",
        "#006d2c",
        "#54278f",
        "#993404",

    ];

    let plots = {};
    let legendItemState = new Map();
    let dispatch = d3.dispatch('legendChange', 'plotItemInspect', 'sampleInspect');
    let tooltip = d3.select('#tooltip');

    let highlightCache = null;
    let highlightedExperimentID = null;
    let highlightedSample = null;

    let plotAdjustmentRequested = false;
    let plotQuery = null;

    function localStorageAvailable() {
        try {
            let x = '__storage_test__';
            window.localStorage.setItem(x, x);
            window.localStorage.removeItem(x);
            return true;
        }
        catch(e) {
            return false;
        }
    }

    function copyStylesInline(destinationNode, sourceNode) {
        let containerElements = ['svg','g'];
        for (let cd = 0; cd < destinationNode.childNodes.length; cd++) {
            let child = destinationNode.childNodes[cd];
            if (containerElements.indexOf(child.tagName) != -1) {
                copyStylesInline(child, sourceNode.childNodes[cd]);
                continue;
            }
            let style = sourceNode.childNodes[cd].currentStyle || window.getComputedStyle(sourceNode.childNodes[cd]);
            if (style == 'undefined' || style == null) continue;
            for (let st = 0; st < style.length; st++){
                child.style.setProperty(style[st], style.getPropertyValue(style[st]));
            }
        }
    }

    function copyPlotForExport(svgElement, colorFunction, plotTitle) {
        let clone = svgElement.cloneNode(true);
        copyStylesInline(clone, svgElement);
        clone.style.background = '#222';

        let d3Clone = d3.select(clone);

        let titleHeight = 30;

        let cloneWidth = Number.parseInt(d3Clone.attr('width'));
        let cloneHeight = Number.parseInt(d3Clone.attr('height'));

        let main = d3Clone.select('g:first-child');
        let oldTransform = main.attr('transform');
        let transformTokens = oldTransform.match(/translate\((\d+),(\d+)\)/);
        let yTranslate = (transformTokens.length === 3) ? Number.parseInt(transformTokens[2]) : 15;
        let newTransform = `translate(${transformTokens[1]}, ${yTranslate + titleHeight})`;
        main.attr('transform', newTransform);
        cloneHeight += yTranslate + titleHeight;

        d3Clone
            .insert('text', ':first-child')
            .attr('id', 'plotTitle')
            .attr('x', cloneWidth * 0.5)
            .attr('y', 20)
            .style('text-anchor', 'middle')
            .style('font-size', '16px')
            .attr('fill', '#fff')
            .attr('font-family', "'Source Sans Pro', sans-serif")
            .text(plotTitle);

        let legendItems = querySelectorAll('.legendItem:not(.targetHidden)')
            .sort(function(a, b) {
                let aSample = a.getAttribute('data-sample');
                let bSample = b.getAttribute('data-sample');
                if (aSample < bSample) {
                    return -1;
                }
                if (aSample > bSample) {
                    return 1;
                }

                return 0;
            });

        let maxSampleLength = legendItems.map(function(i) {return i.getAttribute('data-sample').length;}).reduce(function (a, b) {return a > b ? a : b;});

        let legendItemWidth = maxSampleLength * 15;
        let legendItemsPerLine = Math.floor((cloneWidth - 40)/ legendItemWidth);
        let legendHeight = cloneHeight + (15 * Math.ceil(legendItems.length / legendItemsPerLine));

        let plotHeight = cloneHeight;
        cloneHeight += legendHeight;

        d3Clone
            .insert('rect', ':first-child')
            .attr('fill', '#222')
            .attr('height', cloneHeight)
            .attr('width', cloneWidth);

        d3Clone
            .attr("preserveAspectRatio", "xMinYMin meet")
            .attr('viewBox', '0 0 ' + cloneWidth + ' ' + cloneHeight)
            .attr('height', cloneHeight);

        let legendItemIndex = 0;
        let lineItemIndex = 0;
        let lineCount = 0;
        for (let legendItem of legendItems) {
            if (lineItemIndex > legendItemsPerLine) {
                lineCount += 1;
                lineItemIndex = 0;
            }
            let sampleColor = colorFunction(legendItem.getAttribute('data-sample'));
            d3Clone
                .append('text')
                .attr('class', 'label')
                .attr('x', 20 + lineItemIndex * legendItemWidth)
                .attr('y', plotHeight + (15 * lineCount))
                .style('text-anchor', 'left')
                .style('font-size', '0.75rem')
                .attr('fill', sampleColor)
                .attr('font-family', "'Source Sans Pro', sans-serif")
                .text(legendItem.getAttribute('data-sample'));
            legendItemIndex++;
            lineItemIndex++;
        }
        return clone;
    }

    function triggerDownload (imgURI, fileName) {
        let evt = new MouseEvent('click', {
            view: window,
            bubbles: false,
            cancelable: true
        });
        let a = document.getElementById('exportDownload');
        a.setAttribute('download', fileName);
        a.setAttribute('href', imgURI);
        a.dispatchEvent(evt);
    }

    function showTooltip(header, content) {
        tooltip.html(`<h3><span>${header}</span><button id="tooltipCloser" class="cleaner"><i class="fa fa-times-circle"></i><span></span></button></h3><div class="content">${content}</content>`);
        tooltip.style('z-index', 99999).style('display', 'block');
    }

    function hideTooltip(d, i) {
        tooltip.style('z-index', -99999).style('display', 'none');
    }

    function hideHelp() {
        hideMask();
        for (let visibleHelp of querySelectorAll('.visibleHelp')) {
            visibleHelp.classList.remove('visibleHelp');
            if (localStorageAvailable()) {
                window.localStorage.setItem(visibleHelp.id, 'seen');
            }
        }
    }

    function showHelp(helpID) {
        let help = document.getElementById(helpID);
        if (help) {
            help.classList.add('visibleHelp');
        }
    }

    function hideMask() {
        let mask = document.getElementById('mask');
        mask.classList.remove('active');
    }

    function showMask() {
        let mask = document.getElementById('mask');
        mask.classList.add('active');
    }

    function log(first, ...rest) {
        if (consoleEnabled) {
            console.log(first, ...rest);
        }
    }

    function closest(startNode, className) {
        let parentNode = startNode.parentNode;
        let result = null;
        while (parentNode.parentNode) {
            if (parentNode.classList.contains(className)) {
                result = parentNode;
            }
            parentNode = parentNode.parentNode;
        }
        return result
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

    let formatIntegerForLocale = window.Intl ?
        new Intl.NumberFormat(
            window.navigator.languages || [window.navigator.language || window.navigator.userLanguage],
            {
                localeMatcher: 'best fit',
                maximumFractionDigits: 0
            }
        ).format : formatNumber;

    let formatNumberForLocale = window.Intl ?
        new Intl.NumberFormat(
            window.navigator.languages || [window.navigator.language || window.navigator.userLanguage],
            {
                localeMatcher: 'best fit',
                minimumFractionDigits: 3,
                maximumFractionDigits: 3
            }
        ).format : formatNumber;

    function hrefToID(href) {
        let id = null;
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
        let container;
        selectors = typeof(selectors) == 'string' ? [selectors] : selectors;
        if (elementOrId) {
            container = typeof(elementOrId) == 'string' ? document.getElementById(elementOrId) : elementOrId;
        } else {
            container = document;
        }
        let result = selectors.map(function(selector) {return Array.from(container.querySelectorAll(selector));}).reduce(function(p, c, i, a) {return p.concat(c);}, []);
        return result;
    }

    function getAttribute(object, path) {
        let value = object;
        for (let component of path.split('.'))  {
            value = value[component];
        }
        return value;
    }

    function setAttribute(object, path, value) {
        let components = path.split('.');
        for (let i = 0; i < components.length - 1; i++) {
            let component = components[i];
            if (!(component in object)) {
                object[component] = {};
            }
            object = object[component];
        }
        object[components[components.length - 1]] = value;
    }

    function activateTab(tabID) {
        hideHelp();
        hideTooltip();
        document.getElementById('help').dataset.helpid = tabID + 'Help';
        let tablist = document.getElementById('tabs');
        for (let t of querySelectorAll(['.tab'], tablist)) {
            if (t.dataset.tab === tabID) {
                t.classList.add('active');
            } else {
                t.classList.remove('active');
            }

            let tab = document.getElementById(t.dataset.tab);
            if (tab.id === tabID) {
                tab.classList.add('active');
            } else {
                tab.classList.remove('active');
            }
        }
    }

    function debounce(func, wait) {
        let timeout = null;
        let last = null;
        return function() {
            let now = Date.now();
            let elapsed = now - last;
            if (!timeout || elapsed > wait) {
                last = now;
                let context = this;
                let args = arguments;

                let deferred = function() {
                    requestAnimationFrame(function() {func.apply(context, args);});
                    if (timeout) {
                        clearTimeout(timeout);
                        timeout = null;
                    }
                };

                if (timeout) {
                    clearTimeout(timeout);
                    timeout = null;
                }
                timeout = setTimeout(deferred, wait);
            }
        };
    }

    function addEventListeners() {
        window.addEventListener('resize', debounce(populatePlots, 500));

        for (let tab of querySelectorAll(['.tab a'])) {
            tab.addEventListener('click', function(evt) {
                evt.preventDefault();
                let tabLink = evt.target;
                if (!tabLink.classList.contains('tabLink')) {
                    tabLink = closest(evt.target, 'tabLink');
                }
                let id = hrefToID(tabLink.href);
                if (id) {
                    activateTab(id);
                    return false;
                }
            });
        }

        document.getElementById('mask').addEventListener('click', function(evt) {
            hideHelp();
        }, true);

        document.getElementById('tooltip').addEventListener('click', function(evt) {
            if (evt.target.classList.contains('cleaner') || closest(evt.target, 'cleaner')) {
                hideTooltip();
            }
        }, true);

        for (let helpOpener of querySelectorAll(['.helpOpener'])) {
            helpOpener.addEventListener('click', function(evt) {
                evt.preventDefault();
                showMask();
                let helpID = evt.target.dataset.helpid;
                showHelp(helpID);
            }, true);
        }

        for (let helpCloser of querySelectorAll(['.help .cleaner'])) {
            helpCloser.addEventListener('click', hideHelp, true);
        }

        document.body.addEventListener('keyup', function(evt) {
            if (evt.target.tagName === 'INPUT') {
                return true;
            }

            switch (evt.keyCode) {
            case 27: hideHelp(); hideTooltip(); break;
            case 69: activateTab('experimentTab'); break;
            case 84: activateTab('tableTab'); break;
            case 80: activateTab('plotTab'); break;
            }
        });

        dispatch.on('plotItemInspect' , function() {
            highlightedExperimentID = this.experimentID || null;
            highlightedSample = this.sample || null;

            requestAnimationFrame(highlightPlotItem);
        });

        document.getElementById('plotSearch').addEventListener('change', handlePlotSearchInput);
        document.getElementById('plotSearch').addEventListener('keyup', handlePlotSearchInput);

    }

    function highlightPlotItem() {
        if (highlightedExperimentID) {
            for (let highlightedItem of querySelectorAll(['#plotTab .plotItem.highlight, #plotTab .legendItem.highlight'])) {
                highlightedItem.classList.remove('highlight');
            }

            for (let plotItem of highlightCache.plotItems[highlightedExperimentID]) {
                plotItem.classed('highlight', true);
                plotItem.raise()
            }

            for (let legendItem of highlightCache.legendItems[highlightedSample]) {
                legendItem.classed('highlight', true);
            }
        } else {
            d3.selectAll('.plotItem, .legendItem').classed('highlight', false);
        }
    }

    function adjustPlotItemVisibility() {
        plotAdjustmentRequested = false;
        if (plotQuery) {
            let visibleSamples = [];
            for (let [sample, selected]of legendItemState) {
                if (selected) {
                    visibleSamples.push(sample);
                }
            }

            let selectors = ['sample', 'experiment', 'description'].map(function(a) {return `.plotItem[data-${a}*="${plotQuery}" i]`;});

            for (let plotItem of querySelectorAll(['.plotItem'])) {
                plotItem.classList.add('hidden');
            }

            let matchingPlotItems = querySelectorAll(selectors, document.getElementById('plotTab'));
            for (let plotItem of matchingPlotItems) {
                if (visibleSamples.indexOf(plotItem.getAttribute('data-sample')) != -1) {
                    plotItem.classList.remove('hidden');
                }
            }
        } else {
            for (let [sample, selected] of legendItemState) {
                d3.selectAll('.plotItem[data-sample="' + sample + '"]').classed('hidden', !selected);
                d3.selectAll('.legendItem[data-sample="' + sample + '"]').classed('targetHidden', !selected);
            }
        }
    }

    function setPlotSearchFilter() {
        let changed = false;
        let query = document.getElementById('plotSearch').value;
        if (!query || query === '' && plotQuery) {
            changed = true;
            plotQuery = null;
        } else if (plotQuery === null || query != plotQuery.source) {
            changed = true;
            plotQuery = query.trim();
            document.getElementById('plotSearch').value = plotQuery;
        }
        if (changed) {
            if (!plotAdjustmentRequested) {
                plotAdjustmentRequested = true;
                requestAnimationFrame(adjustPlotItemVisibility);
            }
        }
    }

    function handlePlotSearchInput(evt) {
        evt.stopPropagation();
        window.setTimeout(setPlotSearchFilter, 100);
    }

    function listExperiments() {
        let metadata = [];

        for (let experimentID in configuration.metrics) {
            let experiment_metrics = configuration.metrics[experimentID];
            experiment_metrics.library_description = experiment_metrics.library.description;
            experiment_metrics.description_with_url = experiment_metrics.url ? '<a target="_blank" href="' + experiment_metrics.url + '">' + experiment_metrics.description + '</a>' : experiment_metrics.description;
            experiment_metrics.download_link = '<a href="' + experiment_metrics.metrics_url + '" class="metrics_url download" download>Download</a>';
            metadata.push(experiment_metrics);
        }

        $('#experimentsTable').DataTable( {
            data: metadata,
            columns: [
                {data: 'name'},
                {data: 'library.sample'},
                {data: 'library.library'},
                {data: 'library_description'},
                {data: 'description_with_url'},
                {data: 'download_link', orderable: false, searchable: false}
            ],
            ordering: [[0, 'asc'], [1, 'asc'], [2, 'asc']],
            responsive: true,
            rowId: 'name',
            stateSave: true
        });
    }

    function clearStatus() {
        let msg = document.getElementById('message');
        msg.innerHTML = '';

        let status = document.getElementById('status');
        status.classList.remove('active');
    }

    function setStatus(content, showSpinner) {
        let msg = document.getElementById('message');
        msg.innerHTML = content ? content : '';

        let spinner = document.getElementById('spinner');
        spinner.style.visibility = showSpinner ? 'visible' : 'hidden';

        let status = document.getElementById('status');
        status.classList.add('active');
    }

    function getWidth(el) {
        let width = el.offsetWidth;
        let style = window.getComputedStyle(el);
        width = width - parseInt(style.paddingLeft) - parseInt(style.paddingRight);
        return width;
    }

    function getHeight(el) {
        let height = el.offsetHeight;
        let style = window.getComputedStyle(el);
        height = height - parseInt(style.paddingTop) - parseInt(style.paddingBottom);
        return height;
    }

    function wrapY(text, width, x) {
        text.each(function() {
            let text = d3.select(this),
                words = text.text().split(/\s+/).reverse(),
                word,
                line = [],
                lineNumber = 0,
                lineHeight = 1.1, // ems
                y = text.attr('y') || '0.0',
                dy = parseFloat(text.attr('dy') || '0.0'),
                tspan = text.text(null).append('tspan').attr('x', x).attr('y', y).attr('dy', dy + 'em');

            let yOffset = lineNumber * lineHeight + dy;
            while ((word = words.pop())) {
                line.push(word);
                tspan.text(line.join(' '));
                if (tspan.node().getComputedTextLength() > width) {
                    line.pop();
                    tspan.text(line.join(' '));
                    line = [word];
                    yOffset = ++lineNumber * lineHeight + dy;
                    tspan = text.append('tspan').attr('x', x).attr('y', y).attr('dy', yOffset + 'em').text(word);
                }
            }
        });
    }

    function selectLegendItem(evt) {
        evt.preventDefault();
        let li = evt.target.classList.contains('legendItem') ? evt.target : closest(evt.target, 'legendItem');
        if (li && li.dataset && li.dataset.sample) {
            let sample = li.dataset.sample;
            if (legendItemState.get(sample)) {
                legendItemState.set(sample, false);
            } else {
                legendItemState.set(sample, true);
            }
            dispatch.call('legendChange');
        }
    }

    function changeSampleVisibility() {
        for (let [sample, selected] of legendItemState) {
            d3.selectAll('.legendItem[data-sample="' + sample + '"]').classed('targetHidden', !selected);
            adjustPlotItemVisibility();
        }
    }

    dispatch.on('legendChange', changeSampleVisibility);

    function deselectAllLegendItems() {
        for (let sample of legendItemState.keys()) {
            legendItemState.set(sample, false);
        }
        dispatch.call('legendChange');
    }

    function selectAllLegendItems() {
        for (let sample of legendItemState.keys()) {
            legendItemState.set(sample, true);
        }
        dispatch.call('legendChange');
    }

    function makeFragmentLengthDistancePlot() {
        let containerID = 'fragmentLengthDistancePlot';
        let plotFunction = function() {
            let container = document.getElementById(containerID);

            let plotElement = container.querySelector('.plot');
            plotElement.innerHTML = '';

            let yAxisSelect = container.querySelector('.yaxis-select');
            let yAxisLabel = yAxisSelect.options[yAxisSelect.selectedIndex].innerHTML;

            function getYValue(experiment) {
                let y;
                let yAxisSource = yAxisSelect.value;
                switch(yAxisSource) {
                case 'short_mononucleosomal_ratio':
                case 'tss_enrichment':
                case 'max_fraction_reads_from_single_autosome':
                case 'duplicate_fraction_in_peaks':
                case 'duplicate_fraction_not_in_peaks':
                case 'peak_duplicate_ratio':
                    y = experiment[yAxisSource];
                    break;
                case 'duplicate_autosomal_reads':
                    y = (100.0 * (experiment[yAxisSource] / experiment.total_autosomal_reads));
                    break;
                default:
                    y = (100.0 * (experiment[yAxisSource] / experiment.total_reads));
                }
                return y;
            }


            function makeLibrary(experimentID) {
                let experiment = configuration.metrics[experimentID];
                if (experiment) {
                    return {
                        experimentID: experimentID,
                        library: experiment.library.library || experiment.name,
                        sample: experiment.library.sample || experiment.name,
                        description: experiment.library.description,
                        x: experiment.fragment_length_distance,
                        y: getYValue(experiment)
                    };
                }
            }

            let data = [];
            let samples = new Map();
            let experimentIDs = Object.keys(configuration.metrics).sort();
            for (let e = 0; e < experimentIDs.length; e++) {
                let experimentID = experimentIDs[e];
                let library = makeLibrary(experimentID);
                data.push(library);

                let sample = library.sample;
                if (samples.get(sample)) {
                    samples.get(sample).libraries.push(library);
                } else {
                    samples.set(sample, {libraries: [library]});
                }
            }

            let margin = {top: 15, right: 20, bottom: 100, left: 100};
            let width = getWidth(plotElement) - margin.left - margin.right;
            let height = getHeight(plotElement) - margin.bottom;
            let totalWidth = width + margin.left + margin.right;
            let totalHeight = height + margin.top + margin.bottom;

            // setup x
            let xValue = function(d) {return d.x;};  // data -> value
            let xScale = d3.scaleLinear().range([0, width]);  // value -> display
            let xMap = function(d) {return xScale(xValue(d));};  // data -> display
            let xAxis = d3.axisBottom(xScale).tickSize(-height).tickPadding(5);

            // setup y
            let yValue = function(d) { return d.y;}; // data -> value
            let yScale = d3.scaleLinear().range([height, 0]); // value -> display
            let yMap = function(d) { return yScale(yValue(d));}; // data -> display
            let yAxis = d3.axisLeft(yScale).tickSize(-width);

            // setup fill color
            let cValue = function(d) { return d.sample;};
            let color = d3.scaleOrdinal(plotPalette);
            color.domain([...samples.keys()].sort());

            let limit = Math.max(Math.abs(d3.min(data, xValue)), Math.abs(d3.max(data, xValue))) * 1.05;
            if (d3.min(data, xValue) < 0) {
                xScale.domain([-1 * limit, limit]);
            } else {
                xScale.domain([0, limit]);
            }
            yScale.domain([0, d3.max(data, yValue) * 1.05]);

            let svg = d3.select(plotElement).append('svg')
                .attr('class', 'plotRoot')
                .attr('width', totalWidth)
                .attr('height', totalHeight)
                .attr('viewBox', '0 0 ' + totalWidth + ' ' + totalHeight)
                .attr('preserveAspectRatio', 'xMinYMin');

            let main = svg.append('g')
                .attr('transform', 'translate(' + margin.left + ',' + margin.top + ')');

            main.append('rect')
                .attr('fill', 'rgba(255, 255, 255, 0)')
                .attr('class', 'plotBackground')
                .attr('width', width)
                .attr('height', height);

            // y-axis
            let yAxisG = main.append('g')
                .attr('class', 'y axis')
                .call(yAxis);

            yAxisG.append('text')
                .attr('class', 'label')
                .attr('transform', 'rotate(-90) translate(0, -20)')
                .attr('x', height * -0.5)
                .attr('y', -50)
                .style('text-anchor', 'middle')
                .attr('font-family', "'Source Sans Pro', sans-serif")
                .text(yAxisLabel);

            yAxisG.selectAll('text.label').call(wrapY, height, height * -0.5);

            // x-axis
            let xAxisG = main.append('g')
                .attr('class', 'x axis')
                .attr('transform', 'translate(0,' + height + ')')
                .call(xAxis);

            xAxisG.append('text')
                .attr('class', 'label')
                .attr('x', width * 0.5)
                .attr('y', 60)
                .style('text-anchor', 'middle')
                .attr('font-family', "'Source Sans Pro', sans-serif")
                .text('Distance from reference fragment length distribution');

            if (d3.min(data, xValue) < 0) {
                svg.selectAll('.x .tick')
                    .classed('origin',function(d, i){
                        return (d == 0);
                    });
            }

            let dots = main.append('svg')
                .attr('width', width)
                .attr('height', height);

            // draw dots
            dots.selectAll('.dot')
                .data(data)
                .enter().append('circle')
                .attr('id', function(d) {return `${containerID}-${d.experimentID}`})
                .attr('class', function(d, i) {
                    let classes = 'dot plotItem';
                    if (!legendItemState.get(d.sample)) {
                        classes += ' hidden';
                    }
                    return classes;
                })
                .attr('data-sample', function(d) {return d.sample;})
                .attr('data-experiment', function(d) {return d.experimentID;})
                .attr('data-description', function(d) {return d.description;})
                .attr('r', 5.5)
                .attr('cx', xMap)
                .attr('cy', yMap)
                .style('fill', function(d) { return color(cValue(d));})
                .on('click', function(d) {
                    // d3.select(d3.event.target).lower();
                    showTooltip(
                        `Library ${d.experimentID}`,
                        `<table><tbody>
                         <tr><th>Library</th><td>${d.library}</td></tr>
                         <tr><th>Sample</th><td>${d.sample}</td></tr>
                         <tr><th>Description</th><td>${d.description}</td></tr>
                         <tr><th>${yAxisLabel}</th><td>${formatNumberForLocale(d.y)}</td></tr>
                         <tr><th>Distance</th><td>${d3.format('.10g')(d.x)}</td></tr>
                         </tbody></table>`
                    );
                })
                .on('mouseover', function(d) {
                    dispatch.call('plotItemInspect', d);
                })
                .on('mouseout', function() {dispatch.call('plotItemInspect', null);});

            dots.selectAll('.dot').each(function(d, i) {d3.select(this).append('title').text(d.experimentID);});

            function zoomed() {
                let transform = d3.event.transform;

                // rescale the x linear scale so that we can draw the x axis
                xAxis.scale(transform.rescaleX(xScale));
                xAxisG.call(xAxis);

                // rescale the y linear scale so that we can draw the y axis
                yAxis.scale(transform.rescaleY(yScale));
                yAxisG.call(yAxis);

                // draw the dots in their new positions
                let dots = svg.selectAll('.dot');
                let newR = 5.5 - Math.log(transform.k);
                dots.attr('transform', transform)
                    .attr('r', newR);
            }

            let zoom = d3.zoom()
                .scaleExtent([1, 40])
                .on('zoom', zoomed);

            svg.call(zoom);

            function reset() {
                svg.transition()
                    .duration(500)
                    .call(zoom.transform, d3.zoomIdentity);
            }

            container.querySelector('.reset').addEventListener('click', reset, true);

            function exportPNG() {
                let plotTitle = container.querySelector('h2').innerText;
                let fileName = plotTitle + '.png';
                let svgElement = plotElement.querySelector('.plotRoot');
                let copy = copyPlotForExport(svgElement, color, plotTitle);

                let canvas = document.getElementById('exportCanvas')
                let svgBox = svgElement.getBBox();
                let svgWidth = svgBox.width;
                let svgHeight = svgBox.height;
                canvas.width = svgWidth * 4;
                canvas.height = (svgHeight * 1.25) * 4;

                let ctx = canvas.getContext("2d");
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                let data = (new XMLSerializer()).serializeToString(copy);
                let DOMURL = window.URL || window.webkitURL || window;
                let img = new Image();
                let svgBlob = new Blob([data], {type: "image/svg+xml;charset=utf-8"});
                let url = DOMURL.createObjectURL(svgBlob);
                img.onload = function () {
                    ctx.fillRect(0, 0, canvas.width, canvas.height);

                    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
                    DOMURL.revokeObjectURL(url);
                    if (typeof navigator !== "undefined" && navigator.msSaveOrOpenBlob)
                    {
                        let blob = canvas.msToBlob();
                        navigator.msSaveOrOpenBlob(blob, fileName);
                    }
                    else {
                        let imgURI = canvas
                            .toDataURL("image/png")
                            .replace("image/png", "image/octet-stream");
                        triggerDownload(imgURI, fileName);
                    }
                };
                img.src = url;
            }

            d3.select(container).select('.exportPNG').on('click', exportPNG);

            function exportSVG() {
                let plotTitle = container.querySelector('h2').innerText;
                let fileName = plotTitle + '.svg';
                let svgElement = plotElement.querySelector('.plotRoot');

                let copy = copyPlotForExport(svgElement, color, plotTitle);

                let DOMURL = window.URL || window.webkitURL || window;
                let data = (new XMLSerializer()).serializeToString(copy);
                let svgBlob = new Blob([data], {type: "image/svg+xml;charset=utf-8"});
                let url = DOMURL.createObjectURL(svgBlob);
                triggerDownload(url, fileName);
            }

            d3.select(container).select('.exportSVG').on('click', exportSVG);

            d3.select(container).select('.helpOpener').on('click', function() {
                let helpID = this.dataset.helpid;
                showHelp(helpID);
            });

        };
        plotFunction.containerID = containerID;
        return plotFunction;
    }

    plots.plotFragmentLengthDistance = makeFragmentLengthDistancePlot();

    function makeLinePlot(containerID, provideData) {
        //
        // containerID is the ID of a .plotContainer
        //
        // provideData is a function that returns an object with these properties:
        //    series -- an array of objects, each containing:
        //              - experimentID
        //              - library
        //              - description
        //              - sampleID
        //              - line (an array of objects containing the properties x and y)
        //
        //    xMax -- the extent of the x axis
        //    yMax -- the extent of the y axis
        //    xLabel -- the label for the x axis
        //    yLabel -- the label for the y axis
        //

        let plotFunction = function() {
            let container = document.getElementById(containerID);

            let plotElement = container.querySelector('.plot');
            plotElement.innerHTML = '';

            let data = provideData(containerID);

            if (!data) {
                container.remove();
                return;
            }

            let samples = new Map();
            let experimentIDs = Object.keys(configuration.metrics).sort();
            for (let experimentID of experimentIDs) {
                let datum = data.series[experimentID];

                if (samples.get(datum.sample)) {
                    samples.get(datum.sample).libraries.push(datum);
                } else {
                    samples.set(datum.sample, {libraries: [datum]});
                }
            }

            let margin = {top: 15, right: 20, bottom: 100, left: 100};
            let width = getWidth(plotElement) - margin.left - margin.right;
            let height = getHeight(plotElement) - margin.bottom;
            let totalWidth = width + margin.left + margin.right;
            let totalHeight = height + margin.top + margin.bottom;

            // setup x
            let xValue = function(d) {return d.x;};
            let xScale = d3.scaleLinear().range([0, width]).domain([data.xMin, data.xMax]);
            let xMap = function(d) {return xScale(xValue(d));};
            let xAxis = d3.axisBottom(xScale).tickSize(-height).tickPadding(10);

            // setup y
            let yValue = function(d) { return d.y;};

            let yScaleSelect = container.querySelector('.yScale');
            let yScaleExponent = yScaleSelect ? Number.parseFloat(yScaleSelect.value) : 1;

            // The scale is switched by changing the exponent;
            // d3.scalePow with exponent 1 is effectively linear.
            let yScale = d3.scalePow().exponent(yScaleExponent).range([height, 0]).domain([data.yMin, data.yMax * 1.05]);
            let yMap = function(d) { return yScale(yValue(d));};
            let yAxis = d3.axisLeft(yScale).tickSize(-width).ticks(5);

            let color = d3.scaleOrdinal(plotPalette);
            color.domain([...samples.keys()].sort());

            let svg = d3.select(plotElement).append('svg')
                .attr('class', 'plotRoot')
                .attr('width', totalWidth)
                .attr('height', totalHeight)
                .attr('viewBox', '0 0 ' + totalWidth + ' ' + totalHeight)
                .attr('preserveAspectRatio', 'xMinYMin');

            let main = svg.append('g')
                .attr('transform', 'translate(' + margin.left + ',' + margin.top + ')');

            main.append('rect')
                .attr('class', 'plotBackground')
                .attr('width', width)
                .attr('height', height);

            // x-axis
            let xAxisG = main.append('g')
                .attr('class', 'x axis')
                .attr('transform', 'translate(0,' + height + ')')
                .call(xAxis);

            xAxisG.append('text')
                .attr('class', 'label')
                .attr('x', width * 0.5)
                .attr('y', 60)
                .attr('fill', '#eee')
                .style('text-anchor', 'middle')
                .attr('font-family', "'Source Sans Pro', sans-serif")
                .text(data.xLabel);

            // y-axis
            let yAxisG = main.append('g')
                .attr('class', 'y axis')
                .call(yAxis);

            yAxisG.append('text')
                .attr('class', 'label')
                .attr('transform', 'rotate(-90) translate(0, -' + (margin.left - 20) + ')')
                .style('text-anchor', 'middle')
                .attr('font-family', "'Source Sans Pro', sans-serif")
                .text(data.yLabel);

            yAxisG.selectAll('text.label').call(wrapY, height, height * -0.5);

            function zoomed() {
                let transform = d3.event.transform;

                // rescale the x linear scale so that we can draw the x axis
                xAxis.scale(transform.rescaleX(xScale));
                xAxisG.call(xAxis);

                // rescale the y linear scale so that we can draw the y axis
                yAxis.scale(transform.rescaleY(yScale));
                yAxisG.call(yAxis);

                // draw the lines in their new positions
                let lines = svg.selectAll('.line');
                lines.attr('transform', transform);
            }

            let zoom = d3.zoom()
                .scaleExtent([0.5, 2])
                .on('zoom', zoomed);

            svg.call(zoom);

            // draw lines
            let line = d3.line()
                .curve(d3.curveBasis)
                .x(xMap)
                .y(yMap);

            let plot = main.append('svg')
                .attr('width', width)
                .attr('height', height);

            for (let experimentID of experimentIDs) {
                let datum = data.series[experimentID];
                plot.append('path')
                    .datum(datum.line)
                    .attr('id', `${containerID}-${datum.experimentID}`)
                    .attr('class', function(d) {
                        let classes = 'line plotItem';
                        if (!legendItemState.get(datum.sample)) {
                            classes += ' hidden';
                        }
                        return classes;
                    })
                    .attr('data-sample', datum.sample)
                    .attr('data-experiment', datum.experimentID)
                    .attr('data-description', function(d) {return datum.description;})
                    .attr('d', line)
                    .style('fill', 'none')
                    .style('stroke', color(datum.sample))

                    .on('click', makeClickHandler(datum))
                    .on('mouseover', makeMouseoverHandler(datum))
                    .on('mouseout', handleMouseout)
                    .append('title').text(datum.experimentID);
            }

            let referenceToggle = container.querySelector('.referenceToggle');
            let showReference = referenceToggle ? referenceToggle.checked : true;

            if (data.reference_series) {
                plot.append('path')
                    .datum(data.reference_series.line)
                    .attr('class', function() {
                        return 'line reference ' + (showReference ? '' : 'hidden');
                    })
                    .attr('d', line)
                    .style('fill', 'none')
                    .on('click', function(d) {
                        d3.select(d3.event.target).lower();
                    });
            }

            function makeClickHandler(d) {
                return function() {
                    showTooltip(
                        `Library ${d.experimentID}`,
                        `<table><tbody>
                            <tr><th>Library</th><td>${d.library}</td></tr>
                            <tr><th>Sample</th><td>${d.sample}</td></tr>
                            <tr><th>Description</th><td>${d.description}</td></tr>
                            </tbody></table>`
                    );
                }
            }

            function makeMouseoverHandler(d) {
                return function() {
                    dispatch.call('plotItemInspect', d);
                };
            }

            function handleMouseout() {
                dispatch.call('plotItemInspect', null);
            }

            function reset() {
                svg.transition()
                    .duration(500)
                    .call(zoom.transform, d3.zoomIdentity);
            }
            d3.select(container).select('.reset').on('click', reset);

            function exportPNG() {
                let plotTitle = container.querySelector('h2').innerText;
                let fileName = plotTitle + '.png';
                let svgElement = plotElement.querySelector('.plotRoot');
                let copy = copyPlotForExport(svgElement, color, plotTitle);

                let canvas = document.getElementById('exportCanvas')
                let svgBox = svgElement.getBBox();
                let svgWidth = svgBox.width;
                let svgHeight = svgBox.height;
                canvas.width = svgWidth * 4;
                canvas.height = (svgHeight * 1.25) * 4;

                let ctx = canvas.getContext("2d");
                ctx.clearRect(0, 0, canvas.width, canvas.height);
                let data = (new XMLSerializer()).serializeToString(copy);
                let DOMURL = window.URL || window.webkitURL || window;
                let img = new Image();
                let svgBlob = new Blob([data], {type: "image/svg+xml;charset=utf-8"});
                let url = DOMURL.createObjectURL(svgBlob);
                img.onload = function () {
                    ctx.fillRect(0, 0, canvas.width, canvas.height);

                    ctx.drawImage(img, 0, 0, canvas.width, canvas.height);
                    DOMURL.revokeObjectURL(url);
                    if (typeof navigator !== "undefined" && navigator.msSaveOrOpenBlob)
                    {
                        let blob = canvas.msToBlob();
                        navigator.msSaveOrOpenBlob(blob, fileName);
                    }
                    else {
                        let imgURI = canvas
                            .toDataURL("image/png")
                            .replace("image/png", "image/octet-stream");
                        triggerDownload(imgURI, fileName);
                    }
                };
                img.src = url;
            }

            d3.select(container).select('.exportPNG').on('click', exportPNG);

            function exportSVG() {
                let plotTitle = container.querySelector('h2').innerText;
                let fileName = plotTitle + '.svg';
                let svgElement = plotElement.querySelector('.plotRoot');

                let copy = copyPlotForExport(svgElement, color, plotTitle);

                let DOMURL = window.URL || window.webkitURL || window;
                let data = (new XMLSerializer()).serializeToString(copy);
                let svgBlob = new Blob([data], {type: "image/svg+xml;charset=utf-8"});
                let url = DOMURL.createObjectURL(svgBlob);
                triggerDownload(url, fileName);
            }

            d3.select(container).select('.exportSVG').on('click', exportSVG);

            d3.select(container).select('.helpOpener').on('click', function() {
                let helpID = this.dataset.helpid;
                showHelp(helpID);
            });
        }
        plotFunction.containerID = containerID;
        return plotFunction;
    }

    plots.plotFragmentLength = makeLinePlot(
        'fragmentLengthPlot',
        function(containerID) {
            let container = document.getElementById(containerID);
            let experimentIDs = Object.keys(configuration.metrics).sort();

            let fragment_length_counts = [];
            for (let experimentID of experimentIDs) {
                let m = configuration.metrics[experimentID];
                fragment_length_counts.push(Object.keys(m['fragment_length_counts']).length);
            }
            let maximumInterestingFragmentLength = d3.min(fragment_length_counts);

            let result = {
                series: {},
                reference_series: {
                    source: configuration.fragment_length_reference.source,
                    line: []
                },
                xMin: 0,
                xMax: maximumInterestingFragmentLength,
                yMin: 0,
                yMax: 0.0,
                xLabel: 'Fragment length (bp)',
                yLabel: 'Fraction of all reads'
            };

            let resolutionSelect = container.querySelector('.resolution');
            let resolution = resolutionSelect ? Number.parseInt(resolutionSelect.value) : 10;

            for (let e = 0; e < experimentIDs.length; e++) {
                let experimentID = experimentIDs[e];
                let experiment = configuration.metrics[experimentID];

                result.series[experimentID] = {
                    experimentID: experimentID,
                    library: experiment.library.library || experiment.name,
                    description: experiment.library.description,
                    sample: (experiment.library.sample || experiment.name),
                    line: []
                };

                let sum = 0;
                for (let fl = 0; fl < maximumInterestingFragmentLength; fl++) {
                    let fractionOfReadCount = (experiment.fragment_length_counts[fl][1] || 0);

                    sum += fractionOfReadCount;

                    if (fl % resolution == 0) {
                        let mean = sum / resolution;
                        if (mean > result.yMax) {
                            result.yMax = mean;
                        }

                        result.series[experimentID].line.push({
                            x: fl,
                            y: mean
                        });
                        sum = 0;
                    }
                }
            }

            // add reference distribution
            let sum = 0;
            for (let fl = 0; fl < maximumInterestingFragmentLength; fl++) {
                let fractionOfReadCount = (configuration.fragment_length_reference.distribution[fl][1] || 0);

                sum += fractionOfReadCount;

                if (fl % resolution == 0) {
                    let mean = sum / resolution;
                    if (mean > result.yMax) {
                        result.yMax = mean;
                    }

                    result.reference_series.line.push({
                        x: fl,
                        y: mean
                    });
                    sum = 0;
                }
            }

            return result;
        }
    );

    plots.plotTSSEnrichment = makeLinePlot(
        'tssEnrichmentPlot',
        function(containerID) {
            let experimentIDs = Object.keys(configuration.metrics).sort();

            let result = {
                series: {},
                xMin: 0.0,
                xMax: 0.0,
                yMin: 0.0,
                yMax: 0.0,
                xLabel: 'Position relative to TSS',
                yLabel: 'Enrichment'
            };

            let container = document.getElementById(containerID);
            let resolutionSelect = container.querySelector('.resolution');
            let resolution = resolutionSelect ? Number.parseInt(resolutionSelect.value) : 10;

            let tss_coverage_present = false;
            for (let e = 0; e < experimentIDs.length; e++) {
                let experimentID = experimentIDs[e];
                let experiment = configuration.metrics[experimentID];

                result.series[experimentID] = {
                    experimentID: experimentID,
                    library: experiment.library.library,
                    description: experiment.library.description,
                    sample: (experiment.library.sample || experiment.name),
                    line: []
                };

                if (experiment.tss_coverage) {
                    tss_coverage_present = true;
                    let tssRegionSize = ((experiment.tss_coverage.length - 1) / 2);
                    if (0 - tssRegionSize < result.xMin) {
                        result.xMin = 0 - tssRegionSize;
                    }
                    result.xMax = tssRegionSize;

                    let index = 0;
                    let mean = 0.0;
                    for (let position of experiment.tss_coverage) {
                        index++;
                        mean += position[1];
                        if (index % resolution == 0) {
                            mean /= resolution;
                            result.series[experimentID].line.push({
                                x: index - tssRegionSize - 1,
                                y: mean
                            });

                            if (mean > result.yMax) {
                                result.yMax = mean;
                            }
                            mean = 0.0;
                        }
                    }
                }
            }
            if (tss_coverage_present === false) {
                result = null;
            }
            return result;
        }
    );

    plots.plotPeakReadCounts = makeLinePlot(
        'peakReadCountsPlot',
        function(containerID) {
            let experimentIDs = Object.keys(configuration.metrics).sort();

            let result = {
                reference_series: {
                    source: configuration.reference_peak_metrics.source,
                    line: []
                },
                series: {},
                xMin: 0,
                xMax: 100,
                yMin: 0,
                yMax: 0.0,
                xLabel: 'Peak percentile',
                yLabel: 'Cumulative fraction of high-quality autosomal reads'
            };

            let peaks_present = false;
            for (let e = 0; e < experimentIDs.length; e++) {
                let experimentID = experimentIDs[e];
                let experiment = configuration.metrics[experimentID];

                result.series[experimentID] = {
                    experimentID: experimentID,
                    library: experiment.library.library,
                    description: experiment.library.description,
                    sample: (experiment.library.sample || experiment.name),
                    line: []
                };

                if (experiment.peak_percentiles && experiment.peak_percentiles.cumulative_fraction_of_hqaa.length != 0) {
                    peaks_present = true;
                }

                let percentiles = experiment.peak_percentiles
                    ? experiment.peak_percentiles.cumulative_fraction_of_hqaa
                    : new Array(100).fill(0);

                let cumulativeFractionOfHQAA = 0.0;
                for (let percentile = 0; percentile < 100; percentile++) {
                    cumulativeFractionOfHQAA = percentile < percentiles.length ? percentiles[percentile] : cumulativeFractionOfHQAA;

                    if (cumulativeFractionOfHQAA > result.yMax) {
                        result.yMax = cumulativeFractionOfHQAA;
                    }

                    result.series[experimentID].line.push({
                        x: percentile + 1,
                        y: cumulativeFractionOfHQAA
                    });
                }
            }

            // add reference distribution
            for (let p = 0; p < 100; p++) {
                let fraction = configuration.reference_peak_metrics.cumulative_fraction_of_hqaa[p] || 0;
                if (fraction > result.yMax) {
                    result.yMax = fraction;
                }

                result.reference_series.line.push({
                    x: p + 1,
                    y: fraction
                });
            }

            if (peaks_present === false) {
                result = null;
            }
            return result;
        },
        '<p>The dashed red line represents the reference distribution. You can toggle it on and off.</p>'
    );

    plots.plotPeakTerritory = makeLinePlot(
        'peakTerritoryPlot',
        function(containerID) {
            let experimentIDs = Object.keys(configuration.metrics).sort();

            let result = {
                reference_series: {
                    source: configuration.reference_peak_metrics.source,
                    line: []
                },
                series: {},
                xMin: 0,
                xMax: 100,
                yMin: 0,
                yMax: 0.0,
                xLabel: 'Peak percentile',
                yLabel: 'Cumulative fraction of peak territory'
            };

            let peaks_present = false;
            for (let e = 0; e < experimentIDs.length; e++) {
                let experimentID = experimentIDs[e];
                let experiment = configuration.metrics[experimentID];

                result.series[experimentID] = {
                    experimentID: experimentID,
                    library: experiment.library.library,
                    description: experiment.library.description,
                    sample: (experiment.library.sample || experiment.name),
                    line: []
                };

                if (experiment.peak_percentiles && experiment.peak_percentiles.cumulative_fraction_of_territory.length != 0) {
                    peaks_present = true;
                }

                let percentiles = experiment.peak_percentiles
                    ? experiment.peak_percentiles.cumulative_fraction_of_territory
                    : new Array(100).fill(0);


                let cumulativeFractionOfTerritory = 0.0;
                for (let percentile = 0; percentile < 100; percentile++) {
                    cumulativeFractionOfTerritory = percentile < percentiles.length ? percentiles[percentile] : cumulativeFractionOfTerritory;

                    if (cumulativeFractionOfTerritory > result.yMax) {
                        result.yMax = cumulativeFractionOfTerritory;
                    }

                    result.series[experimentID].line.push({
                        x: percentile + 1,
                        y: cumulativeFractionOfTerritory
                    });
                }
            }

            // add reference distribution
            for (let p = 0; p < 100; p++) {
                let fraction = configuration.reference_peak_metrics.cumulative_fraction_of_territory[p] || 0;
                if (fraction > result.yMax) {
                    result.yMax = fraction;
                }

                result.reference_series.line.push({
                    x: p + 1,
                    y: fraction
                });
            }

            if (peaks_present === false) {
                result = null;
            }
            return result;
        },
        '<p>The dashed red line represents the reference distribution. You can toggle it on and off.</p>'
    );

    plots.plotMapq = makeLinePlot(
        'mapqPlot',
        function(containerID) {
            let experimentIDs = Object.keys(configuration.metrics).sort();

            let result = {
                series: {},
                xMin: 0,
                xMax: 0.0,
                yMin: 0,
                yMax: 0.0,
                xLabel: 'Mapping quality',
                yLabel: 'Fraction of all reads'
            };

            for (let e = 0; e < experimentIDs.length; e++) {
                let experimentID = experimentIDs[e];
                let experiment = configuration.metrics[experimentID];

                result.series[experimentID] = {
                    experimentID: experimentID,
                    library: experiment.library.library,
                    description: experiment.library.description,
                    sample: (experiment.library.sample || experiment.name),
                    line: []
                };

                let totalReads = experiment.total_reads;
                if (experiment.mapq_counts) {
                    for (let mqc of experiment.mapq_counts) {
                        if (mqc[0] > result.xMax) {
                            result.xMax = mqc[0];
                        }

                        let normed = mqc[1] / totalReads;

                        if (normed > result.yMax) {
                            result.yMax = normed;
                        }

                        result.series[experimentID].line.push({
                            x: mqc[0],
                            y: normed
                        });
                    }
                }
            }
            return result;
        }
    );

    function populateTables() {
        let tableMetrics = JSON.parse(JSON.stringify(configuration.metrics));

        for (let table of querySelectorAll('table.data')) {
            let keys = [];
            let order =[];
            let columns = [];

            for (let headerCell of querySelectorAll(['th[data-metric]'], table)) {
                let key = headerCell.dataset.metric;
		    if (configuration.less_redundant && headerCell.className == 'redundant') {
			continue;
		    }
                keys.push(key);

                let col = {
                    data: key,
                    className: headerCell.className
                };
                columns.push(col);

                if (headerCell.dataset.order) {
                    let cellOrder = headerCell.dataset.order.split(':');
                    order.push([Number.parseInt(cellOrder[0]), cellOrder[1]]);
                }
            }
            order.sort();

            let values = [];
            for (let experimentID of Object.keys(tableMetrics).sort()) {
                let experiment = tableMetrics[experimentID];

                for (let key of keys) {
                    let value = getAttribute(experiment, key);

                    if (typeof(value) == 'number') {
                        if (Number.isInteger && Number.isInteger(value)) {
                            value = formatIntegerForLocale(value);
                        } else {
                            value = formatNumberForLocale(value);
                        }
                    }

                    if (experiment.library.description) {
                        setAttribute(experiment, key, '<span data-hovertext="' + experiment.library.description + '">' + value + '</span>');
                    } else {
                        setAttribute(experiment, key, value);
                    }
                }
                values.push(experiment);
            }

            $('#' + table.id).DataTable({
                data: values,
                buttons: [
                    {
                        extend: 'csv',
                        filename: table.getAttribute('data-export')
                    },
                    {
                        extend: 'excel',
                        filename: table.getAttribute('data-export')
                    },
                ],
                columns: columns,
                dom: 'Bfrtlpi',
                order: order,
                stateSave: true
            });
        }
    }

    function addPlotConfigurationHandlers() {
        for (let plotConfigurationSelect of querySelectorAll('.plot-configuration')) {
            plotConfigurationSelect.addEventListener('change', function() {
                plots[this.dataset.plot]();
                buildHighlightCache();
                adjustPlotItemVisibility();
            }, true);
        }

        for (let referenceToggle of querySelectorAll('.referenceToggle')) {
            referenceToggle.addEventListener('change', function() {
                let selector = '#' + this.dataset.plot + ' .reference';
                d3.selectAll(selector)
                    .classed('hidden', !this.checked);
            });
        }
    }

    function loadExperiments() {
        addPlotConfigurationHandlers();
        window.setTimeout(function() {
            listExperiments();
            populateSampleList();
            window.setTimeout(function() {
                setStatus('Creating plots...', true);
                populatePlots();
                window.setTimeout(function() {
                    setStatus('Creating metrics tables...', true);
                    window.setTimeout(function() {
                        populateTables();
                        window.setTimeout(function() {
                            clearStatus();
                            document.getElementById('tabbodies').style.position = 'relative';
                            document.getElementById('tabbodies').style.top = 0;
                            addEventListeners();
                        }, 10);
                    }, 100);
                }, 100);
            }, 100);
        }, 100);
    }

    function buildHighlightCache() {
        highlightCache = {
            'legendItems': {},
            'plotItems': {}
        }

        d3.selectAll('.legendItem')
            .each(function() {
                let sample = this.getAttribute('data-sample');
                if (highlightCache.legendItems[sample]) {
                    highlightCache.legendItems[sample].push(d3.select(this));
                } else {
                    highlightCache.legendItems[sample] = [d3.select(this)];
                }
            });

        d3.selectAll('.plotItem')
            .each(function() {
                let experimentID = this.getAttribute('data-experiment');
                if (highlightCache.plotItems[experimentID]) {
                    highlightCache.plotItems[experimentID].push(d3.select(this));
                } else {
                    highlightCache.plotItems[experimentID] = [d3.select(this)];
                }
            });
    }

    function populatePlots() {
        plots.plotFragmentLengthDistance();
        plots.plotFragmentLength();
        plots.plotTSSEnrichment();
        plots.plotPeakReadCounts();
        plots.plotPeakTerritory();
        plots.plotMapq();
        buildHighlightCache();
        requestAnimationFrame(setPlotSearchFilter);
    }

    function populateSampleList() {
        let color = d3.scaleOrdinal(plotPalette);

        let samples = new Map();
        let experimentIDs = Object.keys(configuration.metrics).sort();
        for (let e = 0; e < experimentIDs.length; e++) {
            let experimentID = experimentIDs[e];
            let experiment = configuration.metrics[experimentID];
            if (experiment) {
                let sample = experiment.library.sample || experiment.name;
                let library = experiment.library.library || experiment.name;

                if (samples.get(sample)) {
                    samples.get(sample).libraries.push(library);
                } else {
                    samples.set(sample, {libraries: [library]});
                }
            }
        }
        let sampleIDs = [...samples.keys()].sort()

        color.domain(sampleIDs);

        let maxSampleIDLength = sampleIDs.reduce(function (a, b) { return a.length > b.length ? a.length : b.length; });

        let listContainer = document.getElementById('plotSampleList');
        let list = listContainer.querySelector('nav');
        let sampleList = document.createElement('ul');
        document.getElementById('sampleCount').innerHTML = '(' + sampleIDs.length + ')';
        for (let sampleID of sampleIDs) {
            let legendItem = document.createElement('li');
            legendItem.style.flexBasis = (maxSampleIDLength + 2) + 'em';
            legendItem.classList.add('legendItem');
            legendItem.dataset.sample = sampleID;

            let legendItemColor = document.createElement('span');
            legendItemColor.classList.add('legendItemColor');
            legendItemColor.style.backgroundColor = color(sampleID);
            legendItem.appendChild(legendItemColor);

            let legendItemLabel = document.createElement('span');
            legendItemLabel.classList.add('legendItemLabel');
            legendItemLabel.innerHTML = sampleID;
            legendItem.appendChild(legendItemLabel);

            sampleList.appendChild(legendItem);

        }
        list.appendChild(sampleList);

        sampleList.addEventListener('mouseover', function(evt) {
            evt.preventDefault();
            if (evt.target.classList.contains('legendItemColor') || evt.target.classList.contains('legendItemLabel')) {
                let li = evt.target.classList.contains('legendItem') ? evt.target : closest(evt.target, 'legendItem');
                if (li && li.dataset && li.dataset.sample) {
                    dispatch.call('sampleInspect', li.dataset.sample);
                }
            }
        }, true);

        sampleList.addEventListener('mouseout', function(evt) {
            evt.preventDefault();
            if (evt.target.classList.contains('legendItemColor') || evt.target.classList.contains('legendItemLabel')) {
                let li = evt.target.classList.contains('legendItem') ? evt.target : closest(evt.target, 'legendItem');
                if (li && li.dataset && li.dataset.sample) {
                    dispatch.call('sampleInspect', null);
                }
            }
        }, true);

        sampleList.addEventListener('click', selectLegendItem, true);
        d3.select(listContainer).select('.hideall').on('click', deselectAllLegendItems);
        d3.select(listContainer).select('.showall').on('click', selectAllLegendItems);

        dispatch.on('sampleInspect', function() {
            let sample = String(this);
            let s = samples.get(sample);
            if (s) {
                d3.selectAll('.plotItem[data-sample="' + sample + '"]').raise().classed('highlight', true);
            } else {
                d3.selectAll('.plotItem').classed('highlight', false);
            }
            d3.selectAll('.reference').raise();
        });
    }

    function initialize() {
        let bannerHeight = document.getElementById('banner').offsetHeight;
        document.getElementById('main').style.marginTop = (bannerHeight + 20) + 'px';

        if (configuration.description) {
            document.getElementById('description').innerHTML = configuration.description;
        }

        for (let fls of querySelectorAll('.fragment_length_reference_source')) {
            fls.innerHTML = configuration.fragment_length_reference.source;
        }

        for (let experimentID in configuration.metrics) {
            let experiment = configuration.metrics[experimentID];
            legendItemState.set(experiment.library.sample || experiment.name, true);
        }

        loadExperiments();

	if (configuration.less_redundant) {
	    d3.selectAll('.redundant').style("display", "none");
	    $('[colspan=2]').attr("colspan", "1");
	}
    }

    function configure(conf) {
        configuration = conf
    }

    return {
        configure: configure,
        initialize: initialize
    };
})();
