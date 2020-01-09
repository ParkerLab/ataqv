####################################
ataqv: ATAC-seq QC and visualization
####################################

***********
What is it?
***********

A toolkit for measuring and comparing ATAC-seq results, made in the
`Parker lab`_ at the University of Michigan. We wrote it to help us
understand how well our ATAC-seq assays had worked, and to make it
easier to spot differences that might be caused by library prep or
sequencing.

The main program, ``ataqv``, examines your aligned reads and reports some
basic metrics, including:

* reads mapped in proper pairs
* optical or PCR duplicates
* reads mapping to autosomal or mitochondrial references
* the ratio of short to mononucleosomal fragment counts
* mapping quality
* various kinds of problematic alignments

If you also have a file of peaks called on your data, that file can be
examined to report read coverage of the peaks.

With a file of transcription start sites, ataqv can report a TSS
enrichment metric based on the transposition activity around those
locations.

The report is printed as plain text to standard output, and detailed
metrics are written to JSON files for further processing.

A web-based visualization and comparison tool and a script to prepare
the JSON output for it are also provided. The web viewer includes
interactive tables of the metrics and plots of fragment length,
distance from a fragment length reference distribution, mapping
quality, counts of reads overlapping peaks, and peak territory.

Web viewer demo: https://parkerlab.github.io/ataqv/demo/

******************
Where does it run?
******************

It's tested on Linux and Macs. It may compile and run on other UNIX
systems.

****
Help
****

If you have questions or suggestions, mail us at
`parkerlab-software@umich.edu`_, or file a `GitHub issue`_.

***************
Getting started
***************

There are several ways to get ``ataqv`` running on your system:
install a binary package; install it with `Homebrew`_ or `Linuxbrew`_;
or build it from source.

Binary packages (Linux only)
============================

We provide several Linux binary packages under `recent releases on
Github`_. Install ``.deb`` files with ``dpkg``, ``.rpm`` files with
``dnf`` or ``yum``, or download and extract the ``ataqv-x.x.x.tar.gz``
file and add the full path to the resulting ``ataqv-x.x.x/bin``
subdirectory to your PATH environment variable.

Homebrew (Mac or Linux)
=======================

The easiest way to install ataqv from source is via `Homebrew`_ on
Macs, or `Linuxbrew`_ on Linux, using our `tap`_. At a shell prompt::

  brew tap ParkerLab/tap
  brew install ataqv

Building from source manually
=============================

Prerequisites
-------------

To build ataqv, you need:

* Linux or a Mac (it may work on other UNIX systems, but it's untested)
* C++11 compiler (gcc 4.9 or newer, or clang on OS X)
* `Boost`_
* `HTSlib`_

The ``mkarv`` script that collects ataqv results and sets up a web
application to visualize them requires Python 2.7 or newer.

To run the test suite, you'll also need `LCOV`_, which can be
installed via `Homebrew`_ or `Linuxbrew`_.

On Debian-based Linux distributions, you can install dependencies
with::

  sudo apt install libboost-all-dev libhts-dev libncurses5-dev libtinfo-dev zlib1g-dev lcov

and the latest supported option among::

  sudo apt install libstdc++-6-dev
  sudo apt install libstdc++-5-dev
  sudo apt install libstdc++-4.9-dev

Building
--------

At your shell prompt::

  git clone https://github.com/ParkerLab/ataqv
  cd ataqv
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

If you use custom locations like this, you will probably need to set
LD_LIBRARY_PATH for the shared libraries to be found at runtime::

  export LD_LIBRARY_PATH=/path/to/boost/lib:/path/to/htslib/lib:$LD_LIBRARY_PATH

Dependency notes
----------------

Boost
^^^^^

If your Boost installation used their "tagged" layout, the libraries
will include metadata in their names; on Linux this usually just means
that they'll have a ``-mt`` suffix to indicate multithreading
support. Specify ``BOOST_TAGGED=yes`` in your make commands to link
with those.

HTSlib
^^^^^^

If HTSlib was built to use libcurl, you'll need to link with that as
well::

  make HTSLIBCURL=yes

Installation
------------

The Makefile supports the common `DESTDIR` and `prefix` variables. To
install to /usr/local::

  make install prefix=/usr/local

Support for the `Environment Modules`_ system is also included. You
can install to the modules tree by defining the ``MODULES_ROOT`` and
``MODULEFILES_ROOT`` variables. If your modules are kept under
``/opt/modules``, with their accompanying module files under
``/opt/modulefiles``, run::

  make install-module MODULES_ROOT=/opt/modules MODULEFILE_ROOT=/opt/modulefiles

And then you should be able to run ``module load ataqv`` to have
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
ataqv can produce some additional metrics using that.

Verifying ataqv results with data from a variety of common tools is on
our to-do list, but so far, we've only used `bwa`_, `Picard's
MarkDuplicates`_, and `MACS2`_ for these steps. A pipeline like ours
can be generated with the included ``make_ataqv_pipeline`` script. Its
output product starts from a BAM file of aligned reads, marks
duplicates and calls peaks, then runs ataqv and produces a web viewer
for the output.

Running
=======

The main program is ataqv. Run ``ataqv --help`` for complete
instructions.

When run, ataqv prints a human-readable summary to its standard
output, and writes complete metrics to the JSON file named with the
`--metrics-file` option.

The JSON output can be incorporated into a web application that
presents tables and plots of the metrics, and makes it easy to compare
results across samples or experiments. Use the ``mkarv`` script to
create a local instance of the result viewer (run ``mkarv -h`` for complete instructions). A web server is not
required, though you can use one to publish your result viewer
instance.

Given several BAM files (mapped to hg19) and accompanying broadPeak files (along with hg19 TSS files and blacklist), an example workflow might be::

  $ # first, run ataqv on each bam file to generate JSON files as well as human-readable output
  $ ataqv --peak-file /lab/work/porchard/atacseq/macs2/sample_1_peaks.broadPeak --name sample_1 --metrics-file /lab/work/porchard/atacseq/ataqv/sample_1.ataqv.json.gz --excluded-region-file /lab/work/porchard/atacseq/data/mappability/hg19.blacklist.bed.gz --tss-file /lab/work/porchard/atacseq/data/tss/hg19.tss.refseq.bed.gz --ignore-read-groups human /lab/work/porchard/atacseq/mark_duplicates/sample_1.md.bam > /lab/work/porchard/atacseq/ataqv/sample_1.ataqv.out
  $ ataqv --peak-file /lab/work/porchard/atacseq/macs2/sample_2_peaks.broadPeak --name sample_2 --metrics-file /lab/work/porchard/atacseq/ataqv/sample_2.ataqv.json.gz --excluded-region-file /lab/work/porchard/atacseq/data/mappability/hg19.blacklist.bed.gz --tss-file /lab/work/porchard/atacseq/data/tss/hg19.tss.refseq.bed.gz --ignore-read-groups human /lab/work/porchard/atacseq/mark_duplicates/sample_2.md.bam > /lab/work/porchard/atacseq/ataqv/sample_2.ataqv.out
  $ ataqv --peak-file /lab/work/porchard/atacseq/macs2/sample_3_peaks.broadPeak --name sample_3 --metrics-file /lab/work/porchard/atacseq/ataqv/sample_3.ataqv.json.gz --excluded-region-file /lab/work/porchard/atacseq/data/mappability/hg19.blacklist.bed.gz --tss-file /lab/work/porchard/atacseq/data/tss/hg19.tss.refseq.bed.gz --ignore-read-groups human /lab/work/porchard/atacseq/mark_duplicates/sample_3.md.bam > /lab/work/porchard/atacseq/ataqv/sample_3.ataqv.out
  $
  $ # run mkarv on the JSON files to generate the interactive web viewer (in this case, SRR891268 will be used as the reference sample in the viewer):
  $ mkarv my_fantastic_experiment /lab/work/porchard/atacseq/ataqv/sample_1.ataqv.json.gz /lab/work/porchard/atacseq/ataqv/sample_2.ataqv.json.gz /lab/work/porchard/atacseq/ataqv/sample_3.ataqv.json.gz
  $
  $ # to see the viewer, open the file my_fantastic_experiment/index.html in your web browser

Example
=======

The ataqv package includes a script that will set up and run our
entire ATAC-seq pipeline on some sample data.

You'll need to have installed ataqv itself, plus Picard tools,
samtools, and MACS2 to run the pipeline. On a Mac, you can obtain
everything with::

  $ brew install ataqv picard-tools samtools
  $ pip install MACS2

On Linux, installation of the dependencies is probably specific to
your environment and is left as an exercise for the reader. On Debian,
``apt-get install picard-tools samtools`` followed by installing MACS2
with ``pip install MACS2`` should be enough.

Once you have the prerequisite programs installed, you can run the
example pipeline with::

  $ run_ataqv_example /output/path

Comparing your results to others
================================

Part of this project will be publishing ataqv output for as many
ATAC-seq experiments as we can get our hands on, so we can compare
them and learn how changes to the protocol affect the output. Watch
our `GitHub docs`_ for updates.

***********
Performance
***********

It's not currently concurrent, so don't allocate it more than a single
processor. Memory usage should typically be no more than a few hundred
megabytes.

Anecdotally, processing a 41GB BAM file containing 1,126,660,186
alignments of the data from the ATAC-seq paper took just under 20
minutes and 2GB of memory. Adding peak metrics extended the run time
to almost 40 minutes, but it still used the same amount of memory.

.. _Parker lab: http://theparkerlab.org/
.. _Boost: http://www.boost.org/
.. _HTSlib: http://www.htslib.org/
.. _LCOV: http://ltp.sourceforge.net/coverage/lcov.php
.. _Homebrew: http://brew.sh/
.. _Linuxbrew: http://linuxbrew.sh/
.. _tap: https://github.com/ParkerLab/homebrew-tap
.. _Environment Modules: https://en.wikipedia.org/wiki/Environment_Modules_%28software%29
.. _Github issue: https://github.com/ParkerLab/ataqv/issues
.. _recent releases on GitHub: https://github.com/ParkerLab/ataqv/releases
.. _bwa: http://bio-bwa.sourceforge.net/
.. _Picard's MarkDuplicates: https://broadinstitute.github.io/picard/command-line-overview.html#MarkDuplicates
.. _MACS2: https://github.com/taoliu/MACS/
.. _Github docs: https://parkerlab.github.io/ataqv/
.. _parkerlab-software@umich.edu: mailto:parkerlab-software@umich.edu?subject=ataqv
