# Configuration for core Chaste machines

"""Copyright (C) University of Oxford, 2008

University of Oxford means the Chancellor, Masters and Scholars of the
University of Oxford, having an administrative office at Wellington
Square, Oxford OX1 2JD, UK.

This file is part of Chaste.

Chaste is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Chaste is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
License for more details. The offer of Chaste under the terms of the
License is subject to the License being interpreted in accordance with
English Law and subject to any action against the University of Oxford
being under the jurisdiction of the English Courts.

You should have received a copy of the GNU Lesser General Public License
along with Chaste. If not, see <http://www.gnu.org/licenses/>.
"""

import os

petsc_2_2_path = '../../../petsc-2.2.1/'
petsc_2_3_path = '../../../petsc-2.3.2-p10/'
petsc_build_name = 'linux-gnu'
petsc_build_name_profile = 'linux-gnu'
petsc_build_name_optimized = 'linux-gnu-opt'
dealii_path = '../../../deal.II/'
metis_path = '../../../metis-4.0/'
intel_path = '/opt/intel/cc/9.1.039/lib'
icpc='icpc'

other_includepaths = ['../../../xsd-2.3.1-i686-linux-gnu/libxsd',
                      '../../../include']
other_libpaths = ['../../../lib',
                  os.path.join(petsc_2_3_path, 'externalpackages/f2cblaslapack/linux-gnu')]
blas_lapack = ['f2clapack', 'f2cblas']
other_libraries = ['boost_serialization', 'xerces-c', 'z', 'hdf5']

tools = {}
