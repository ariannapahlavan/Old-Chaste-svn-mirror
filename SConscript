import glob
import os

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
elif test_component in ['', toplevel_dir]:
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


# Look for source files that tests depend on under test/.
# We also need to add any subfolders to the CPPPATH, so they are searched
# for #includes.
testsource = []
test_cpppath = []
for dirpath, dirnames, filenames in os.walk('../../test'):
    for dirname in dirnames[:]:
        if dirname in ['.svn', 'data']:
            dirnames.remove(dirname)
        else:
            test_cpppath.append(os.path.join(dirpath, dirname))
    for filename in filenames:
        if filename[-4:] == '.cpp':
            testsource.append(os.path.join(dirpath, filename))

#print test_cpppath, testsource
#print files, testfiles, testsource

# Add test folders to CPPPATH only for this component
if test_cpppath:
    env = env.Copy()
    env.Prepend(CPPPATH=test_cpppath)

# Determine libraries to link against.
# Note that order does matter!
chaste_libs = [toplevel_dir] + comp_deps[toplevel_dir]
all_libs = ['test'+toplevel_dir] + chaste_libs + other_libs


# Build and install the library for this component
env.SharedLibrary(toplevel_dir, files)
#env.Install('#lib', 'lib'+toplevel_dir+'.so')
# Build the test library for this component
env.Library('test'+toplevel_dir, testsource)

# Make test output depend on shared libraries, so if implementation changes
# then tests are re-run.  Choose which line according to taste.
#lib_deps = map(lambda lib: '#lib/lib%s.so' % lib, chaste_libs) # all libs
lib_deps = '#lib/lib%s.so' % toplevel_dir # only this lib
#linklib_deps = map(lambda lib: '#linklib/lib%s.so' % lib, chaste_libs)

# Collect a list of test log files to use as dependencies for the test
# summary generation
test_log_files = []

# Build and run tests of this component
for testfile in testfiles:
    prefix = testfile[:-4]
    test_hpp = File('test/' + testfile)
    runner_cpp = env.Test(prefix+'Runner.cpp', test_hpp)
    obj = env.Object(prefix+'Runner', runner_cpp)
    prog = env.TestProgram(prefix+'Runner', obj,
                TEST_HPP=test_hpp, RUNNER_CPP=runner_cpp[0],
                LIBS = other_libs,# Want just other_libs, not all_libs
                LIBPATH = ['#linklib', '.'] + other_libpaths)
    nlist = env.ListFiles(prefix+'List.txt', obj,
                          RUNNER_EXE=prog[0])
    env.Depends(prog, nlist)
    env.AlwaysBuild(nlist)
    if not compile_only:
        log_file = env.File(prefix+'.log')
        env.Depends(log_file, lib_deps)
        test_log_files.append(log_file)
        env.RunTests(log_file, prefix+'Runner')

Return("test_log_files")
