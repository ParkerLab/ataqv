##################
ataqc: ATAC-seq QC
##################

***********
What is it?
***********

A toolkit for measuring and comparing ATAC-seq results, made in the
`Parker lab`_ at the University of Michigan. We wrote it to help us
understand how well our ATAC-seq assays had worked, and to make it
easier to spot differences that might be caused by library prep or
sequencing.

The main program, ataqc, examines your aligned reads and reports some
basic metrics, including:

* reads mapped in proper pairs
* optical or PCR duplicates
* reads mapping to autosomal or mitochondrial references
* the ratio of short to mononucleosomal fragment counts
* mapping quality
* various kinds of problematic reads

If you also have a file of peaks called on your data, that file can be
examined to report read coverage of the peaks.

The report is printed as plain text to standard output, and detailed
metrics are written to JSON files for further processing.

A web-based visualization and comparison tool and a script to prepare
the JSON output for it are also provided. The web viewer includes
interactive tables of the metrics and plots of fragment length,
mapping quality, counts of reads overlapping peaks, and peak
territory.

Web viewer demo: https://parkerlab.github.io/ataqc/demo/

***************
Getting started
***************

Prerequisites
=============

To build ataqc, you need:

* Linux or a Mac
* C++11 compiler (gcc 4.9 or newer, or clang on OS X)
* `Boost`_
* `HTSlib`_

The ``mkarv`` script that collects ataqc results and sets up a web
application to visualize them requires Python 2.7 or newer.

Getting it running
==================

Mac
---

The easiest way to install ataqc on Macs is via `Homebrew`_ and our
tap. At the terminal::

  brew tap ParkerLab/tap
  brew install ataqc

You can also build from a clone of the git repository, as described
for Linux below.

Linux
-----

At your shell prompt::

  git clone https://github.com/ParkerLab/ataqc
  cd ataqc
  make

If Boost and htslib are not available in default system locations (for
example if you're using environment modules, or compiling in your home
directory) you'll probably need to give ``make`` some hints via the
``CPPFLAGS`` and ``LDFLAGS`` variables::

  make CPPFLAGS="-I/path/to/boost/include -I/path/to/htslib/include" LDFLAGS="-L/path/to/boost/lib -L/path/to/htslib/lib"

If the environment variables ``BOOST_ROOT`` or ``HTSLIB_ROOT`` are set
to directories containing ``include`` and ``lib`` subdirectories, the
compiler configuration can be made simpler::

  make BOOST_ROOT=/path/to/boost HTSLIB_ROOT=/path/to/htslib

Or you can specify directories in BOOST_INCLUDE, BOOST_LIB,
HTSLIB_INCLUDE, and HTSLIB_LIB separately.

Dependency notes
^^^^^^^^^^^^^^^^

Boost
"""""

If your Boost installation used their "tagged" layout, the libraries
will include metadata in their names; on Linux this usually just means
that they'll have a ``-mt`` suffix to indicate multithreading
support. Specify ``BOOST_TAGGED=yes`` in your make commands to link
with those.

HTSlib
""""""

If htslib was built to use libcurl, you'll need to link with that as
well::

  make HTSLIBCURL=yes

Installing
==========

You can just copy ``build/ataqc`` and ``src/scripts/*`` wherever you
like, or run them from your copy of the ataqc repository. If you want
to install them to a bin directory somewhere, for example
/usr/local/bin, you can run::

  make install PREFIX=/usr/local

Support for the `Environment Modules`_ system is also included. You
can install to the modules tree by defining the ``MODULES_ROOT`` and
``MODULEFILES_ROOT`` variables. If your modules are kept under
``/opt/modules``, with their accompanying module files under
``/opt/modulefiles``, run::

  make install-module MODULES_ROOT=/opt/modules MODULEFILE_ROOT=/opt/modulefiles

And then you should be able to run ``module load ataqc`` to have
everything available in your environment.

*****
Usage
*****

Prerequisites
=============

You'll need to have a BAM file containing alignments of your ATAC-seq
reads to your reference genome. If you want accurate duplication
metrics, you'll also need to have marked duplicates in that BAM
file. If you have a BED file containing peaks called on your data,
ataqc can produce some additional metrics using that.

Verifying ataqc results with data from a variety of common tools is on
our to-do list, but so far, we've only used `bwa`_, `Picard's
MarkDuplicates`_, and `MACS2`_ for these steps. An example pipeline is
provided in the included ``qc.sh`` script, which starts with a BAM
file of aligned reads, marks duplicates and calls peaks, then runs
ataqc and produces a web viewer for the output.

Running
=======

The main program is ataqc. Run ``ataqc --help`` for complete
instructions.

When run, ataqc prints a human-readable summary to its standard output,
and writes complete metrics to the file named with the
`--metrics-file` option.

The JSON output can be incorporated into a web application that
presents tables and plots of the metrics, and makes it easy to compare
results across samples or experiments. Use the ``mkarv`` script to
create a local instance of the result viewer. A web server is not
required, though you can use one to publish your result viewer
instance.

Here's a simple example, which assumes you've already aligned your
reads and called peaks on them::

  ataqc --peak-file example.macs2_peaks.broadPeak --metrics-file example.json.gz example.bam > example.ataqc.out

  mkarv examplearv example.json.gz

  open examplearv/index.html  # assuming you're on a Mac -- on Linux try xdg-open or firefox instead of "open"


Comparing your results to others
================================

Part of this project will be publishing ataqc output for as many
ATAC-seq experiments as we can get our hands on, so we can compare
them and learn how changes to the protocol affect the output. Watch
our `GitHub docs`_ for updates.

****
Help
****

If you have questions or suggestions, mail us at `parkerlab-software@umich.edu`_.


.. _Parker lab: http://theparkerlab.org/
.. _Boost: http://www.boost.org/
.. _HTSlib: http://www.htslib.org/
.. _Homebrew: http://brew.sh/
.. _Environment Modules: https://en.wikipedia.org/wiki/Environment_Modules_%28software%29
.. _bwa: http://bio-bwa.sourceforge.net/
.. _Picard's MarkDuplicates: https://broadinstitute.github.io/picard/command-line-overview.html#MarkDuplicates
.. _MACS2: https://github.com/taoliu/MACS/
.. _Github docs: https://parkerlab.github.io/ataqc/
.. _parkerlab-software@umich.edu: mailto:parkerlab-software@umich.edu?subject=ataqc
