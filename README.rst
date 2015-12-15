ataqc: ATAC-seq QC
==================

What is it?
-----------

A collection of utilities for measuring your ATAC-seq results. The
main program, ataqc, examines your aligned reads and reports some
basic checks, including:

* how many reads mapped in proper pairs
* the number of optical or PCR duplicates
* the number of reads mapping to autosomal or mitochondrial references
* the ratio of short to mononucleosomal fragment counts
* mapping quality

If you also have a file of peaks called on your data, that file can
also be examined to report read coverage of the peaks.

It also creates several data files suitable for processing
with R, and scripts are provided to plot these files.

Getting started
---------------

Prerequisites
^^^^^^^^^^^^^

To build ataqc, you need:

* Linux or OS X
* C++11 compiler (gcc 4.7 or newer, or clang on OS X)
* htslib

To run the plotting scripts, you will need R, with the following
packages installed:

* RColorBrewer
* ggplot2
* grid
* gtools
* scales

Building from source
^^^^^^^^^^^^^^^^^^^^

At your shell prompt::

  git clone https://github.com/ParkerLab/ataqc
  cd ataqc
  make

Installing
^^^^^^^^^^

You can just copy build/ataqc and src/scripts/* wherever you like, or
run them from your copy of the ataqc repository. If you want to
install them to a bin directory somewhere, for example /usr/local/bin,
you can run::

  make install PREFIX=/usr/local

Support for the `Environment Modules`_ system is also included. You
can install to the modules tree by defining the ``MODULES_ROOT``
variable. If your modules are kept under ``/opt/modules``, with their
accompanying module files under ``/opt/modulefiles``, run::

  make install MODULES_ROOT=/opt/modules
  make install-module MODULEFILE_ROOT=/opt/modulefiles

And then you should be able to run ``module load ataqc`` to have
everything available in your environment.

Usage
-----

Prerequisites
^^^^^^^^^^^^^

At a minimum, you'll need to have aligned your ATAC-seq reads to your
reference genome. If you want accurate duplication metrics, you'll
also need to have marked duplicates. If you have called peaks on your
data, ataqc can also produce some metrics using them.

Verifying ataqc results with data from a variety of common tools is
on our to-do list, but so far, we've only used bwa, Picard's
MarkDuplicates, and MACS2 for these steps. Our typical pipeline is
represented in the included qc.sh script, which starts with a BAM
file of aligned reads, marks duplicates and calls peaks, then runs the
ataqc programs required to produce plots in PDF format.

If you've already done all the preparation, you have a couple of
options: edit qc.sh to comment or remove the unnecessary steps, or
run the ataqc steps manually.

Running
^^^^^^^

The main program is ataqc. Run ``ataqc --help`` for complete
instructions.

It prints a human readable summary to its standard output. A table of
that information is written to a file (the `.summary` file). A table
of template length metrics is written to the `.template_metrics`
output file. A file of peaks ranked by the number of reads overlapping
them, containing several other metrics, is written to the
`.peak_metrics` file.

These output files can be plotted with three R scripts:

* `plot_summary_metrics.R` plots various basic metrics.
* `plot_template_lengths.R` plots template lengths; usually the result
  is the characteristic ATAC-seq signature of a sharp peak at short
  fragment lengths (around 50nt), then smaller peaks at lengths
  spanning one, two, or three nucleosomes.
* `plot_peak_metrics.R` produces two plots:

  * The first charts peaks ranked by the number of reads overlapping
    them versus the cumulative fraction of uniquely mapped properly
    paired and mapped autosomal reads (which we call "perfect" reads).
  * The second charts peaks' cumulative fraction of total peak
    territory versus their cumulative fraction of perfect reads.

These are most interesting when you have several ATAC-seq results to
compare. Part of this project will be publishing ataqc output for as
many ATAC-seq experiments as we can get our hands on, so we can
compare them and learn how changes to the protocol affect the
output. Watch our GitHub wiki for updates.

.. _Environment Modules: https://en.wikipedia.org/wiki/Environment_Modules_%28software%29

