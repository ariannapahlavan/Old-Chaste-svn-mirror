import glob
import os

Import("*")

# Note that this script is executed from within the build/ folder
curdir = os.getcwd()

# Get our top-level directory
toplevel_dir = os.path.basename(os.path.dirname(curdir))

# Look for .cpp files within the src folder
files = []
for dirpath, dirnames, filenames in os.walk('../src'):
  for filename in filenames:
    if filename[-4:] == '.cpp':
      files.append(os.path.join(dirpath, filename))

# Look for files containing a test suite
# A list of test suites to run will be found in a test/<name>TestPack.txt
# file, one per line.
try:
  packfile = file('../test/ContinuousTestPack.txt', 'r')
  testfiles = map(lambda s: s.strip(), packfile.readlines())
  packfile.close()
except IOError:
  testfiles = []

# Look for source files that tests depend on in test/.
_testsource = os.listdir('../test')
testsource = []
for file in _testsource:
  if file[:4] != 'Test' and file[-4:] == '.cpp':
    testsource.append('test/' + file)
del _testsource

#print files, testfiles, testsource


petsc_libs = ['petscts', 'petscsnes', 'petscksp', 'petscdm', 
              'petscmat', 'petscvec', 'petsc']
chaste_libs = ['global', 'io', 'ode', 'pde', 'coupled']

all_libs = petsc_libs + chaste_libs + ['test'+toplevel_dir]

opt= Environment(ENV = {'PATH' : os.environ['PATH']})
opt.Append(CCFLAGS = petsc_incs+extra_flags)
opt.Append(LINKFLAGS = link_flags)
opt.Append(BOPT = 'g_c++')
opt.Replace(CXX = mpicxx)
opt.Replace(AR = ar)
opt.Replace(CPPPATH = cpppath)

test = Builder(action = 'cxxtest/cxxtestgen.pl --error-printer -o $TARGET $SOURCES')
runtests = Builder(action = './$SOURCE | tee $TARGET')
#runparalleltests = Builder(action = mpirun + ' -np 2 ./$SOURCE | tee $TARGET')

opt['BUILDERS']['Test'] = test
opt['BUILDERS']['RunTests'] = runtests
#opt['BUILDERS']['RunParallelTests'] = runparalleltests

opt['ENV']['LD_LIBRARY_PATH'] = petsc_base+'lib/libg_c++/linux-gnu/'
opt.Library(toplevel_dir, files)
opt.Install('../../lib', 'lib'+toplevel_dir+'.a')
opt.Library('test'+toplevel_dir, testsource)

for testfile in testfiles:
  opt.Test(testfile[:-4]+'Runner.cpp', 'test/' + testfile) 
  opt.Program(testfile[:-4]+'Runner', [testfile[:-4]+'Runner.cpp'],
              LIBS = all_libs,
              LIBPATH = ['../../lib', '.', petsc_libpath])
  opt.RunTests(testfile[:-4]+'.log', testfile[:-4]+'Runner')


# Parallel tests
#if not ARGUMENTS.get('no_parallel', 0):
#  opt.Test('parallelrunner.cpp', paralleltestfiles)
#  opt.Program('paralleltestrunner', ['parallelrunner.cpp'],
#              LIBS=all_libs,
#              LIBPATH=['../../datawriters/build', '.', petsc_libpath])
#  opt.RunParallelTests('parbuild.log', 'paralleltestrunner')
