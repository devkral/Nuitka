#
#     Copyright 2011, Kay Hayen, mailto:kayhayen@gmx.de
#
#     Part of "Nuitka", an optimizing Python compiler that is compatible and
#     integrates with CPython, but also works on its own.
#
#     If you submit Kay Hayen patches to this software in either form, you
#     automatically grant him a copyright assignment to the code, or in the
#     alternative a BSD license to the code, should your jurisdiction prevent
#     this. Obviously it won't affect code that comes to him indirectly or
#     code you don't submit to him.
#
#     This is to reserve my ability to re-license the code at any time, e.g.
#     the PSF. With this version of Nuitka, using it for Closed Source will
#     not be allowed.
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, version 3 of the License.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#     Please leave the whole of this copyright notice intact.
#
""" Module to hide the complexity of using pickle.

It should be simple, but it is not yet. Not all the pickle modules are well behaved.
"""

from nuitka import Constants

# Work around for CPython 3.x removal of cpickle.
try:
    import cPickle as cpickle
except ImportError:
    # False alarm, no double import at all, pylint: disable=W0404
    import pickle as cpickle

# Need to use the pure Python pickle to workaround seeming bugs of cPickle
import pickle

from logging import warning

def getStreamedConstant( constant_value, constant_type ):

    # Note: The marshal module cannot persist all unicode strings and
    # therefore cannot be used.  The cPickle fails to gives reproducible
    # results for some tuples, which needs clarification. In the mean time we
    # are using pickle.
    try:
        saved = pickle.dumps(
            constant_value,
            protocol = 0 if constant_type is unicode else 0
        )
    except TypeError:
        warning( "Problem with persisting constant '%r'." % constant_value )
        raise

    # Check that the constant is restored correctly.
    restored = cpickle.loads( saved )

    assert Constants.compareConstants( restored, constant_value )

    # If we have Python3, we need to make sure, we use UTF8 or else we get into trouble.
    if str is unicode:
        saved = saved.decode( "utf_8" )

    return saved