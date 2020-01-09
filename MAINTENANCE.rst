############################
Release management for ataqv
############################

Creating ataqv installation packages
====================================

Generic tarball
---------------

You can create a simple distribution tarball with::

  make dist

That will create a ``.tar.gz`` file in the ``build`` subdirectory of the
source tree, which can be extracted and used on a system running the
same operating system and with sufficiently compatible versions of
shared library dependencies. To eliminate the shared library
constraints, you can try to build a static distribution with::

  make dist-static

However, static compilation has only been tried on Linux (RHEL 6;
Debian Stretch and Buster), and it may not work at all on your
distribution. You will almost certainly need HTSlib built without cURL
support, as some of the library dependencies are not available as
shared libraries. Supply the path to your custom HTSlib with ``make
HTSLIB_STATIC_DIR=/path dist-static``.

Debian package
--------------

You can produce a Debian package with ``make deb``. To produce a
Debian package with ataqv statically linked, minimizing the risk of a
target system not having the proper versions of shared libraries, try
``make deb-static``. The same caveats and constraints of static
compilation described above apply here as well.

RPM
---

You can package ataqv in an RPM by running ``make rpm``. Again,
there's a static variant you can try with ``make rpm-static`` to
maximize compatibility.

Making a new ataqv release on GitHub
====================================

Visit https://github.com/ParkerLab/ataqv/releases and click the `Draft
a new release` button. Fill in the form, entering the new version and
selecting the branch or commit you wish to tag with it, describing the
content of the release and attaching any binary packages you've built
with the instructions above.
