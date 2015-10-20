/******************************************************************************\

    CAMotics is an Open-Source simulation and CAM software.
    Copyright (C) 2011-2015 Joseph Coffland <joseph@cauldrondevelopment.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

\******************************************************************************/

#include <camotics/Application.h>
#include <camotics/cutsim/ToolPath.h>
#include <camotics/sim/ToolTable.h>

#include <camotics/machine/Machine.h>
#include <camotics/machine/MachinePipeline.h>
#include <camotics/machine/MachineState.h>
#include <camotics/machine/MachineMatrix.h>
#include <camotics/machine/MachineLinearizer.h>
#include <camotics/machine/MachineUnitAdapter.h>
#include <camotics/machine/GCodeMachine.h>

#include <tplang/TPLContext.h>
#include <tplang/Interpreter.h>

#include <cbang/Exception.h>
#include <cbang/ApplicationMain.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/js/Javascript.h>
#include <cbang/config/MinConstraint.h>

#include <boost/version.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
namespace io = boost::iostreams;

#if BOOST_VERSION < 104400
#define BOOST_CLOSE_HANDLE true
#else
#define BOOST_CLOSE_HANDLE io::close_handle
#endif

#include <iostream>
#include <vector>

using namespace std;
using namespace cb;
using namespace tplang;
using namespace CAMotics;


namespace CAMotics {
  class TPLangApp : public Application {
    ToolTable tools;
    MachinePipeline pipeline;

    string out;
    bool force;
    bool metric;

    SmartPointer<ostream> stream;

  public:
    TPLangApp() :
      Application("Tool Path Language"), out("-"), force(false), metric(true) {
      cmdLine.addTarget("out", out, "Filename for GCode output or '-' to write "
                        "to the standard output stream");
      cmdLine.addTarget("force", force, "Force overwriting output file", 'f');
      cmdLine.addTarget("metric", metric, "Output in metric units.");
      cmdLine.add("imperial", 0, this, &TPLangApp::imperialAction,
                  "Output in imperial units.")->setType(Option::BOOLEAN_TYPE);

      Option &opt =
        *cmdLine.add("pipe", "Specify a output file descriptor, overrides "
                     "the 'out' option");
      opt.setType(Option::INTEGER_TYPE);
      opt.setConstraint(new MinConstraint<int>(0));
    }

    // From cb::Application
    int init(int argc, char *argv[]) {
      int ret = cb::Application::init(argc, argv);
      if (ret == -1) return ret;

      if (cmdLine["--pipe"].hasValue()) {
#ifdef _WIN32
        typedef void *handle_t;
#else
        typedef int handle_t;
#endif

        handle_t pipe = (handle_t)cmdLine["--pipe"].toInteger();
        stream = new io::stream<io::file_descriptor>(pipe, BOOST_CLOSE_HANDLE);

      } else if (out == "-") stream = SmartPointer<ostream>::Phony(&cout);
      else {
        if (SystemUtilities::exists(out) && !force)
          THROWS("File '" << out << "' already exists");
        stream = SystemUtilities::oopen(out);
      }

      Units units = metric ? Units::METRIC : Units::IMPERIAL;

      // Build machine pipeline
      pipeline.add(new MachineUnitAdapter(Units::METRIC, units));
      pipeline.add(new MachineLinearizer);
      pipeline.add(new MachineMatrix);
      pipeline.add(new GCodeMachine(*stream, units));
      pipeline.add(new MachineState);

      return ret;
    }


    void requestExit() {
      Application::requestExit();
      cb::js::Javascript::terminate();
    }

    // From cb::Reader
    void read(const cb::InputSource &source) {
      tplang::TPLContext ctx(cout, pipeline, tools);
      tplang::Interpreter(ctx).read(source);
      stream->flush();
      cout.flush();
    }


    int imperialAction(Option &opt) {
      metric = !opt.toBoolean();
      return 0;
    }
  };
}


int main(int argc, char *argv[]) {
  // Look for v8 args after --
  bool foundV8Args = false;

  for (int i = 1; i < argc; i++)
    if (string("--") == argv[i]) {
      vector<char *> args;

      args.push_back(argv[0]);
      for (int j = i + 1; j < argc; j++) args.push_back(argv[j]);

      int v8Argc = argc - i + 1;
      cb::js::Javascript::init(&v8Argc, &args[0]);

      argc = i;
    }

  if (!foundV8Args) cb::js::Javascript::init(0, 0);

  return cb::doApplication<CAMotics::TPLangApp>(argc, argv);
}
