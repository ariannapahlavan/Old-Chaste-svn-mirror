# Chaste tests display script.

# This module contains most of the functionality, and is loaded from the
# repository by a wrapper script.

import os, time, pysvn


#####################################################################
##                          Webpages                               ##
#####################################################################

def index(req):
  """The main test display page.
  
  This displays a summary of the most recent tests.
  """
  page_body = """\
  <h1>Chaste Tests</h1>
  <p>
  This is the funky new interface to Chaste's testing suite.
  </p>
  <ul>
    <li><a href="%s/recent?type=continuous">Recent continuous builds.</a></li>
    <li><a href="%s/recent?type=nightly">Recent nightly builds.</a></li>
  </ul>
  <h2>Latest continuous build</h2>
""" % (_our_url, _our_url) + _summary(req, type='continuous', revision='last')
  
  return _header() + page_body + _footer()


def testsuite(req, type, revision, machine, buildType, testsuite, status):
  """
  Display the results for the given testsuite, by passing the file back
  to the user.
  """
  req.content_type = 'text/html'
  req.write(_header(), 0)
  test_set_dir = _testResultsDir(type, revision, machine, buildType)
  testsuite_file = os.path.join(test_set_dir, testsuite+'.'+status)
  if os.path.isfile(testsuite_file):
    req.write('\n<pre>\n', 0)
    req.sendfile(testsuite_file)
    req.write('\n</pre>\n', 0)
  else:
    req.write(_error('The requested test suite was not found.'))
  req.write(_footer())


def recent(req, type=''):
  "User-facing page. Content is generated by _recent."
  title = 'Recent '+type+' builds'
  page_body = """\
  <h1>%s</h1>
%s
""" % (title, _recent(req, type))
  return _header(title) + page_body + _footer()

def _recent(req, type=None):
  """Display brief summaries of recent builds of the given type.
  
  Returns a string representing part of a webpage.
  """
  if not type:
    return _error('No type of test to summarise specified.')

  dir = os.path.join(_tests_dir, type)
  if not os.path.isdir(dir):
    return _error(type+' is not a valid type of test.')
  
  # Parse the directory structure within dir into a list of builds
  builds = []
  revisions = os.listdir(dir)
  for revision in revisions:
    machinesAndBuildTypes = os.listdir(os.path.join(dir, revision))
    for machineAndBuildType in machinesAndBuildTypes:
      st = os.stat(os.path.join(dir, revision, machineAndBuildType))
      mod_time = st.st_mtime
      machine, buildType = _extractDotSeparatedPair(machineAndBuildType)
      builds.append([mod_time, int(revision), buildType, machine])
  # Sort the list to be most recent first
  builds.sort()
  builds.reverse()

  output = """\
  <table border="1">
    <tr>
      <th>Date</th>
      <th>Revision</th>
      <th>Build Type</th>
      <th>Machine</th>
      <th>Status</th>
    </tr>
"""
  for build in builds:
    if type == 'nightly':
      date = time.strftime('%d/%m/%Y', time.localtime(build[0]))
    else:
      date = time.strftime('%d/%m/%Y %H:%M:%S', time.localtime(build[0]))
    revision, machine, buildType = build[1], build[3], build[2]
    test_set_dir = _testResultsDir(type, revision, machine, buildType)
    testsuite_status, overall_status, colour = _getTestStatus(test_set_dir,
                                                              buildType)
    
    output = output + """\
    <tr>
      <td>%s</td>
      <td>%s</td>
      <td>%s</td>
      <td>%s</td>
      <td style="background-color: %s;">%s</td>
    </tr>
""" % (date, _linkRevision(revision), _linkBuildType(buildType, revision),
       machine, colour, _linkSummary(overall_status, type, revision,
                                     machine, buildType))
  output = output + "  </table>\n"
  
  return output



def summary(req, type, revision, machine, buildType):
  "User-facing page. Content is generated by _summary."
  page_body = """\
  <h1>Build Summary</h1>
""" +  _summary(req, type, revision, machine, buildType)
  return _header('Test Summary') + page_body + _footer()
  
def _summary(req, type, revision, machine=None, buildType=None):
  """Display a summary of a build.
  
  Returns a string representing part of a webpage.
  """
  if not (type and revision):
    return _error('No test set to summarise specified.')
  
  # If type=='continuous' and the magic revision 'last' is passed,
  # look for the latest revision present.
  if type == 'continuous' and revision == 'last':
    revisions = os.listdir(os.path.join(_tests_dir, type))
    revision = max(revisions)
  
  # There is only one continuous build per revision, so we don't
  # specify the machine & buildType as arguments.
  if type == 'continuous':
    test_set_dir = os.path.join(_tests_dir, type, revision)
    builds = os.listdir(test_set_dir)
    if len(builds) < 1:
      return _error('No test set found for revision '+revision+'.')
    else:
      machine, buildType = _extractDotSeparatedPair(builds[0])
  else:
    if not (machine and buildType):
      return _error('No test set to summarise specified.')
  test_set_dir = _testResultsDir(type, revision, machine, buildType)
  
  # Now test_set_dir should be the directory containing the test results
  # to summarise. Extract summary info from the filenames.
  testsuite_status, overall_status, colour = _getTestStatus(test_set_dir,
                                                            buildType)
  
  # Produce output HTML
  output = """\
  <p>
  Revision: %s<br />
  Overall status: %s<br />
  Build type: %s<br />
  Machine: %s
  </p>
  <table border="1">
    <tr>
      <th>Test Suite</th>
      <th>Status</th>
    </tr>
""" % (_linkRevision(revision), _colourText(overall_status, colour),
       _linkBuildType(buildType, revision), machine)
  
  # Display the status of each test suite, in alphabetical order
  testsuites = testsuite_status.keys()
  testsuites.sort()
  for testsuite in testsuites:
    output = output + """\
    <tr>
      <td>%s</td>
      <td style="background-color: %s;">%s</td>
    </tr>
""" % (testsuite, _statusColour(testsuite_status[testsuite], buildType),
       _linkTestSuite(type, revision, machine, buildType, testsuite,
                      testsuite_status[testsuite]))

  output = output + "  </table>\n"
  
  return output


def buildType(req, buildType, revision=None):
  """
  Display information on the compiler settings, etc. used to build a set
  of tests.
  buildType is the user-friendly name describing these settings, such as
  can be passed to scons build=buildType.
  revision is the code revision of the set of tests, in case the 
  definition of buildType has changed since.
  """
  if revision is None:
    revision = pysvn.Revision(pysvn.opt_revision_kind.head)
  else:
    revision = pysvn.Revision(pysvn.opt_revision_kind.number, int(revision))
  BuildTypes = _importBuildTypesModule(revision)
  build = BuildTypes.GetBuildType(buildType)
  page_body = """\
  <h1>Explanation of build type '%s'</h1>
  <p>
  C++ compiler 'brand': %s<br />
  C++ extra compile flags: %s<br />
  Extra linker flags: %s<br />
  Test packs run: %s<br />
  </p>
""" % (buildType, build.CompilerType(), build.CcFlags(), build.LinkFlags(),
       str(build.TestPacks()))
  return _header() + page_body + _footer()


#####################################################################
##                    Helper functions.                            ##
#####################################################################

def _importModuleFromSvn(module_name, module_filepath,
                         revision=pysvn.Revision(pysvn.opt_revision_kind.head)):
  """
  Use pysvn and imp to import the requested revision of the given
    module from the repository.
  module_name is the name to give the module.
  module_filepath is the path to the module file within the trunk
    directory of the repository.
  By default import the latest version.
  Return the module object.
  """
  filepath = _svn_repos + module_filepath
  client = _svnClient()
  module_text = client.cat(filepath, revision)
  return _importCode(module_text, module_name)

def _importBuildTypesModule(revision=pysvn.Revision(pysvn.opt_revision_kind.head)):
  """
  Use pysvn and imp to import the requested revision of the BuildTypes.py
  module from the repository.
  By default import the latest version.
  """
  filepath = _svn_repos + '/python/BuildTypes.py'
  client = _svnClient()  
  module_text = client.cat(filepath, revision)
  return _importCode(module_text, 'BuildTypes')

def _svnClient():
  "Return a pysvn.Client object for communicating with the svn repository."
  client = pysvn.Client()
  def get_login(realm, username, may_save):
    return True, _svn_user, _svn_pass, True
  client.callback_get_login = get_login
  # We trust our server even though it has a dodgy certificate
  # Might want to make this a bit more secure?
  def ssl_server_trust_prompt(trust_dict):
    return True, trust_dict['failures'], True
  client.callback_ssl_server_trust_prompt = ssl_server_trust_prompt
  return client

def _importCode(code, name, add_to_sys_modules=0):
  """
  Import dynamically generated code as a module. code is the
  object containing the code (a string, a file handle or an
  actual compiled code object, same types as accepted by an
  exec statement). The name is the name to give to the module,
  and the final argument says wheter to add it to sys.modules
  or not. If it is added, a subsequent import statement using
  name will return this module. If it is not added to sys.modules
  import will try to load it in the normal fashion.
  Code from the Python Cookbook.
  
  import foo
  
  is equivalent to
  
  foofile = open("/path/to/foo.py")
  foo = importCode(foofile,"foo",1)
  
  Returns a newly generated module.
  """
  import sys, imp
  
  module = imp.new_module(name)
  
  exec code in module.__dict__
  if add_to_sys_modules:
    sys.modules[name] = module
    
  return module

def _extractDotSeparatedPair(string):
  """
  Extract both parts from a string of the form part1.part2.
  Returns a pair (part1, part2).
  Useful for parsing machine.buildType and TestSuite.Status filenames.
  """
  i = string.find('.')
  return string[:i], string[i+1:]

def _testResultsDir(type, revision, machine, buildType):
  """
  Return the directory in which test results are stored for this
  test type, code revision, build machine and build type.
  """
  return os.path.join(_tests_dir, type, str(revision), machine+'.'+buildType)

def _getTestStatus(test_set_dir, buildType):
  """
  Return the status for all tests in the given directory, and compute
  a summary status given the build type.
  Return a triple (dict, string, string) where the first entry maps
  test suite names to a string describing their status, the second is
  the overall status, and the third is the colour in which to display
  the overall status.
  """
  result_files = os.listdir(test_set_dir)
  testsuite_status = {}
  for filename in result_files:
    testsuite, status = _extractDotSeparatedPair(filename)
    testsuite_status[testsuite] = status
  overall_status, colour = _overallStatus(testsuite_status.values(),
                                          buildType)
  return testsuite_status, overall_status, colour

def _overallStatus(statuses, buildType):
  """
  Given a list of the status of each test suite, and the type of build
  performed, return the overall status.
  Return value is a pair, the first item of which is a string given
  the number of failing test suites, and the second a colour name.
  """
  total = len(statuses)
  failed = 0
  for status in statuses:
    # TODO: Use buildType to determine if status is bad
    if int(status): failed = failed + 1
  if failed > 0:
    result = "Failed %d out of %d test suites" % (failed, total)
    colour = "red"
  else:
    result = "All tests passed"
    colour = "green"
  return result, colour

def _statusColour(status, buildType):
  """
  Return the name of the colour in which this status string should be
  displayed, given that the build type was buildType.
  """
  # TODO: Use buildType
  if int(status):
    colour = "red"
  else:
    colour = "green"
  return colour

#####################################################################
##                   HTML helper functions.                        ##
#####################################################################

def _linkRevision(revision):
  "Return a link tag to the source browser for this revision."
  return '<a href="%s?rev=%s">%s</a>' % (_source_browser_url,
                                         revision, revision)

def _linkBuildType(buildType, revision):
  "Return a link tag to the detailed info page for this build type."
  query = 'buildType?buildType=%s&revision=%s' % (buildType, revision)
  return '<a href="%s/%s">%s</a>' % (_our_url, query, buildType)

def _linkSummary(text, type, revision, machine, buildType):
  """
  Return a link tag to the summary page for this set of tests.
  text is the text of the link.
  """
  query = 'type=%s&revision=%s&machine=%s&buildType=%s' % (type, revision,
                                                           machine, buildType)
  return '<a href="%s/summary?%s">%s</a>' % (_our_url, query, text)

def _linkTestSuite(type, revision, machine, buildType, testsuite, status):
  """
  Return a link tag to a page displaying the output from a single
  test suite.
  """
  query = 'type=%s&revision=%s&machine=%s&buildType=%s' % (type, revision,
                                                           machine, buildType)
  query = query + '&testsuite=%s&status=%s' % (testsuite, status)
  return '<a href="%s/testsuite?%s">%s</a>' % (_our_url, query, status)

def _colourText(text, colour):
  "Return text in the given colour."
  return '<span style="color: %s;">%s</span>' % (colour, text)

def _error(msg):
  "Encapsulate an error message."
  return '<p class="error">%s</p>' % msg

def _header(title=""):
  """HTML page header."""
  if title:
    title = " - " + title
  header = """\
<html>
  <head>
    <title>Chaste Tests%s</title>
  </head>
  <body>""" % title
  return header

def _footer():
  """HTML page footer."""
  footer = """\
  <hr />
  <a href="%s">Tests index page</a><br />
  <a href="https://comlab2.lsi.ox.ac.uk/cgi-bin/trac.cgi">Chaste project website</a>
  </body>
</html>""" % _our_url
  return footer
