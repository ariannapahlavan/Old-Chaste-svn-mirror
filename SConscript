import glob
import os

# Faster shared library linking
import sys
sys.path.append('../../../python')
import fasterSharedLibrary

# Compatability with Python 2.3
try:
  set = set
except NameError:
  import sets
  set = sets.Set

Import("*")

# Note that this script is executed from within the build/<something>/ folder
curdir = os.getcwd()

# Get our top-level directory
toplevel_dir = os.path.basename(os.path.dirname(os.path.dirname(curdir)))

#print curdir, toplevel_dir

# Look for .cpp files within the src folder
os.chdir('../..') # This is so .o files are built in `toplevel_dir'/build/<something>/
files = []
for dirpath, dirnames, filenames in os.walk('src'):
  for filename in filenames:
    if filename[-4:] == '.cpp':
      files.append(os.path.join(dirpath, filename))
os.chdir(curdir)

# Look for files containing a test suite
# A list of test suites to run will be found in a test/<name>TestPack.txt
# file, one per line.
# Alternatively, a single test suite may have been specified on the command
# line.
testfiles = set()
if single_test_suite:
  if single_test_suite_dir == toplevel_dir:
    testfiles.add(single_test_suite)
    # Remove any old test output file to force a re-run
    try:
      os.remove(single_test_suite[:-4] + '.log')
    except OSError:
      pass
else:
  packfiles = []
  if all_tests:
    for packfile in glob.glob('../../test/*TestPack.txt'):
      try:
        packfiles.append(file(packfile, 'r'))
      except IOError:
        pass
  else:
    for testpack in build.TestPacks():
      try:
        packfile = '../../test/'+testpack+'TestPack.txt'
        packfiles.append(file(packfile, 'r'))
      except IOError:
        pass
  for packfile in packfiles:
    try:
      for testfile in map(lambda s: s.strip(), packfile.readlines()):
        # Ignore blank lines and repeated tests.
        if testfile and not testfile in testfiles:
          testfiles.add(testfile)
      packfile.close()
    except IOError:
      pass


# Look for source files that tests depend on in test/.
testsource = []
for file in os.listdir('../../test'):
  if file[-4:] == '.cpp':
    testsource.append('test/' + file)

#print files, testfiles, testsource


# Determine libraries to link against.
# Note that order does matter!
petsc_libs = ['petscts', 'petscsnes', 'petscksp', 'petscdm', 
              'petscmat', 'petscvec', 'petsc']
chaste_libs = ['io', 'ode', 'pde', 'coupled', 'linalg', 'mesh',
               'models', 'global']
all_libs = chaste_libs + petsc_libs + other_libs + ['test'+toplevel_dir]


# Set up build environment
opt = Environment(
  ENV = {'PATH': os.environ['PATH'],
         'USER': os.environ['USER'],
         'CHASTE_TEST_OUTPUT':
         os.environ.get('CHASTE_TEST_OUTPUT',
                        '/tmp/'+os.environ['USER']+'/testoutput/')})
opt['ENV']['LD_LIBRARY_PATH'] = ':'.join(other_libpaths) + ':' + os.path.abspath('../../../lib')
opt.Append(CCFLAGS = '-I' + ' -I'.join(other_includepaths)
           + ' ' + extra_flags)
opt.Append(LINKFLAGS = link_flags)
opt.Append(BOPT = 'g_c++')
opt.Replace(CXX = mpicxx)
opt.Replace(AR = ar)
opt.Replace(CPPPATH = cpppath)

# Create Builders for generating test .cpp files, and running test executables
test = Builder(action = 'cxxtest/cxxtestgen.py --error-printer -o $TARGET $SOURCES')
runtests = Builder(action = 'python/TestRunner.py $SOURCE $TARGET ' +
                   build_type + ' ' + build.GetTestReportDir() + 
                   ' ' + run_time_flags)

opt['BUILDERS']['Test'] = test
opt['BUILDERS']['RunTests'] = runtests

# New shared library builder
opt['BUILDERS']['OriginalSharedLibrary'] = opt['BUILDERS']['SharedLibrary']
opt['BUILDERS']['SharedLibrary'] = fasterSharedLibrary.fasterSharedLibrary

# Build and install the library for this component
opt.SharedLibrary(toplevel_dir, files)
#opt.Install('../../../lib', 'lib'+toplevel_dir+'.a')
# Build the test library for this component
opt.Library('test'+toplevel_dir, testsource)

# Make test output depend on shared libraries, so if implementation changes
# then tests are re-run.  Choose which line according to taste.
#lib_deps = map(lambda lib: '#lib/lib%s.so' % lib, chaste_libs) # all libs
lib_deps = '#lib/lib%s.so' % toplevel_dir # only this lib

# Collect a list of test log files to use as dependencies for the test
# summary generation
test_log_files = []

# Build and run tests of this component
for testfile in testfiles:
  prefix = testfile[:-4]
  opt.Test(prefix+'Runner.cpp', 'test/' + testfile) 
  opt.Program(testfile[:-4]+'Runner', [prefix+'Runner.cpp'],
              LIBS = all_libs,
              LIBPATH = ['../../../linklib', '.'] + other_libpaths)
  if not compile_only:
    log_file = opt.File(prefix+'.log')
    opt.Depends(log_file, lib_deps)
    test_log_files.append(log_file)
    opt.RunTests(log_file, prefix+'Runner')

Return("test_log_files")
