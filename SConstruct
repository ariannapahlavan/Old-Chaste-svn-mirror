# Controlling scons build script for Chaste

# This script is executed within the root Chaste source directory.
# We need at least Python 2.3.
EnsurePythonVersion(2,3)

import sys
import os
import glob
import socket 

sys.path.append('python')
import BuildTypes

sys.path.append('python/hostconfig')
import hostconfig

# The type of build to perform (see python/BuildTypes.py for options)
build_type = ARGUMENTS.get('build', 'default')
build = BuildTypes.GetBuildType(build_type)
build.SetRevision(ARGUMENTS.get('revision', ''))
Export('build')

# Specify test_summary=0 to scons to *NOT* generate a summary html page
test_summary = ARGUMENTS.get('test_summary', 1)

# Specifying extra run-time flags
run_time_flags = ARGUMENTS.get('run_time_flags', '')

# Specify all_tests=1 to select all tests for running (useful with
# compile_only=1)
all_tests = ARGUMENTS.get('all_tests', 0)
Export('all_tests')

# Specify compile_only=1 to not run any tests
compile_only = ARGUMENTS.get('compile_only', 0)
Export('compile_only')

# To run a single test suite only, give its path (relative to the Chaste
# root) as the test_suite=<path> argument.
# This will force the test suite to be run even if the source is unchanged.
single_test_suite = ARGUMENTS.get('test_suite', '')
if single_test_suite:
    single_test_suite = single_test_suite.split(os.path.sep)
    if (len(single_test_suite)<2):
        sys.stderr.write('Path to test suite is too short')
        sys.exit(0)
    if  single_test_suite[-2]!='test' :
        sys.stderr.write('Test suite is not in a  test folder')
        sys.exit(0)
    single_test_suite_dir = single_test_suite[-3]
    single_test_suite = single_test_suite[-1]
    #print single_test_suite, single_test_suite_dir
else:
    single_test_suite_dir = ''
Export('single_test_suite', 'single_test_suite_dir')

# To run tests of only a single component, specify it with the
# test_component=<component> argument.
test_component = ARGUMENTS.get('test_component', '')
Export('test_component')


# Use a single file to store signatures.
# Forwards-compatible with SCons 0.97.
SConsignFile('.sconsign')


# Chaste components (top level dirs).
# We hard code dependencies between them, and use this to work out the
# order to link them in.  Each one is linked against just its dependencies,
# in the order given here.
comp_deps = {'cancer': ['ode', 'mesh', 'linalg', 'io', 'global'],
			 'dealii': ['cancer', 'coupled', 'pde', 'ode', 'mesh', 'linalg', 'io', 'global'],
			 'coupled': ['pde', 'ode', 'mesh', 'linalg', 'io', 'global'],
			 'pde': ['mesh', 'linalg', 'io', 'global'],
			 'mesh': ['linalg', 'global'],
			 'linalg': ['global'],
			 'ode': ['linalg', 'io', 'global'],
			 'io': ['global'],
			 'global': []}
components = ['cancer', 'coupled', 'pde', 'ode',
               'mesh', 'linalg', 'io', 'global']
if build.using_dealii:
    components = ['dealii'] + components
Export('components', 'comp_deps')

Alias('core', Split('global io linalg mesh ode pde'))

# Set extra paths to search for libraries and include files.
# Paths to PETSc, and any other external libraries, should be set here.
# The three variables exported are:
#  other_libs: names of libraries to link against.  Do not include PETSc
#              libraries.  Do not include the 'lib' prefix or any suffixes
#              in the library name (e.g. f2cblas not libf2cblas.a)
#  other_libpaths: paths to search for libraries to link against.  These
#                  should all be absolute paths, for safety.  Do include
#                  the path to PETSc libraries (unless they're in a standard
#                  system location).
#  other_includepaths: paths to search for header files.  Do include the
#                      path to PETSc headers (unless it's standard).
# This is now done by the hostconfig subsystem.
hostconfig.configure(build)
other_libs = hostconfig.libraries
other_libpaths = hostconfig.libpaths
other_includepaths = hostconfig.incpaths


Export("other_libpaths", "other_libs")


## Any extra CCFLAGS and LINKFLAGS
extra_flags = build.CcFlags() + ' ' + hostconfig.ccflags() + ' '
link_flags  = build.LinkFlags()

# Hack to get around Debian sarge strangeness
##if system_name in ['maths']:
##    extra_flags = extra_flags + " -DCWD_HACK "

# Search path for Chaste #includes
cpppath = ['#/', '#/cxxtest']
src_folders = glob.glob('*/src')
for src_folder in src_folders:
    cpppath.append('#/'+src_folder)
    for dirpath, dirnames, filenames in os.walk(src_folder):
        for dirname in dirnames[:]:
            if dirname == '.svn':
                dirnames.remove(dirname)
            else:
                cpppath.append('#/'+os.path.join(dirpath, dirname))


# Set up the environment to use for building.
other_libpaths.append(os.path.abspath('lib'))
env = Environment(
    ENV={'PATH': os.environ['PATH'],
         'PYTHONPATH': os.environ.get('PYTHONPATH', ''),
         'USER': os.environ['USER'],
         'INTEL_LICENSE_FILE': '28518@lic1.osc.ox.ac.uk',
         'CHASTE_TEST_OUTPUT':
         os.environ.get('CHASTE_TEST_OUTPUT',
                        '/tmp/'+os.environ['USER']+'/testoutput/'),
         'LD_LIBRARY_PATH': ':'.join(other_libpaths)})
env.Append(CCFLAGS = '-isystem ' + ' -isystem '.join(other_includepaths)
           + ' ' + extra_flags)
env.Append(LINKFLAGS = link_flags)
env.Append(BOPT = 'g_c++')
env.Replace(CXX = build.tools['mpicxx'])
env.Replace(AR = build.tools['ar'])
env.Replace(CPPPATH = cpppath)
env['buildsig'] = build.GetSignature()

# Create Builders for generating test .cpp files, and running test executables
test = Builder(action = 'cxxtest/cxxtestgen.py --error-printer -o $TARGET $SOURCES')
#runtests = Builder(action = 'python/TestRunner.py $SOURCE $TARGET ' +
#                   build_type + ' ' + run_time_flags)
import TestRunner
def test_description(target, source, env):
    return "running '%s'" % (source[0])
test_action = Action(TestRunner.get_build_function(build, run_time_flags),
                     test_description, ['buildsig'])
runtests = Builder(action=test_action)
env['BUILDERS']['Test'] = test
env['BUILDERS']['RunTests'] = runtests

# Faster builds of shared libraries
import fasterSharedLibrary
fasterSharedLibrary.Copy = Copy
env['BUILDERS']['OriginalSharedLibrary'] = env['BUILDERS']['SharedLibrary']
env['BUILDERS']['SharedLibrary'] = fasterSharedLibrary.fasterSharedLibrary

# Find full path to valgrind, as parallel memory testing needs it to be
# given explicitly.
vg_path = env.WhereIs(build.tools['valgrind'])
if vg_path:
    build.tools['valgrind'] = vg_path
del vg_path

# Export the build environment to SConscript files
Export('env')

# Check for orphaned test files
os.system('python/TestRunner.py python/CheckForOrphanedTests.py ' +
          build.GetTestReportDir() + 'OrphanedTests.log ' + build_type + ' --no-stdout')
# Check for duplicate file names in multiple directories
os.system('python/TestRunner.py python/CheckForDuplicateFileNames.py ' +
          build.GetTestReportDir() + 'DuplicateFileNames.log ' + build_type + ' --no-stdout')

build_dir = build.build_dir
test_depends = [File(build.GetTestReportDir() + 'OrphanedTests.log'),
                File(build.GetTestReportDir() + 'DuplicateFileNames.log')]
for toplevel_dir in components:
    bld_dir = os.path.join(toplevel_dir, 'build', build_dir)
    if not os.path.exists(bld_dir):
        os.mkdir(bld_dir)
    test_depends.append(SConscript('SConscript', src_dir=toplevel_dir, build_dir=bld_dir,
                                   duplicate=0))


# Remove the contents of build.GetTestReportDir() on a clean build
test_output_files = glob.glob(build.GetTestReportDir() + '*')
Clean('.', test_output_files)
# Also remove the entire build.build_dir for each component, so we
# don't have stale tests, etc. still present
for toplevel_dir in components:
    Clean('.', os.path.join(toplevel_dir, 'build', build_dir))
# Also make sure we remove any libraries still hanging around, just in case
for lib in glob.glob('lib/*'):
    Clean('.', lib)
for lib in glob.glob('linklib/*'):
    Clean('.', lib)



# Test summary generation
if test_summary and not compile_only:
  # Copy the build env, since we change TargetSigs
  senv = env.Copy()
  # Get the directory to put results & summary in
  output_dir = build.output_dir
  # Remove old results. Note that this command gets run before anything is built.
  #for oldfile in os.listdir(output_dir):
  #  os.remove(os.path.join(output_dir, oldfile))
  # Add a summary generator to the list of things for scons to do
  if isinstance(build, BuildTypes.Coverage):
    # Remove old .gcda files before running more tests
    # First, find appropriate build directories
    build_dirs = glob.glob('*/build/' + build.build_dir)
    # Now find & remove .gcda files within there.
    # Also remove .log files so tests are re-run
    for build_dir in build_dirs:
      for dirpath, dirnames, filenames in os.walk(build_dir):
        for filename in filenames:
          if filename[-5:] == '.gcda' or filename[-4:] == '.log':
            os.remove(os.path.join(dirpath, filename))
    # For a Coverage build, run gcov & summarise instead
    summary = Builder(action = 'python python/DisplayCoverage.py ' + output_dir+' '+build_type)
  else:
    summary = Builder(action = 'python python/DisplayTests.py '+output_dir+' '+build_type)
  def output_dir_lister(target, source, env, output_dir=output_dir):
    """Create a file containing a directory listing."""
    files = filter(lambda f: f[0] != '.', os.listdir(output_dir))
    files.sort()
    fp = file(str(target[0]), 'w')
    for f in files:
      fp.write(f + '\n')
    fp.close()
    return None # Successful build
  lister = Action(output_dir_lister,
                  lambda ts, ss, env: "Generating file list %s" % ts[0])
  senv['BUILDERS']['TestSummary'] = summary
  # Make sure we use the file list contents as its signature
  senv.TargetSignatures('content')
  file_list = File(os.path.join(output_dir, '.filelist'))
  senv.AlwaysBuild(file_list)
  senv.Command(file_list, test_depends, lister)
  summary_index = os.path.join(output_dir, 'index.html')
  senv.TestSummary(summary_index, [file_list, test_depends])
  senv.AlwaysBuild(summary_index)

