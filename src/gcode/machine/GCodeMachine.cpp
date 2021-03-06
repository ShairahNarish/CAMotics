/******************************************************************************\

    CAMotics is an Open-Source simulation and CAM software.
    Copyright (C) 2011-2017 Joseph Coffland <joseph@cauldrondevelopment.com>

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

#include "GCodeMachine.h"

#include <cbang/Exception.h>
#include <cbang/Math.h>
#include <cbang/net/URI.h>
#include <cbang/log/Logger.h>

#include <limits>

using namespace cb;
using namespace std;
using namespace GCode;


namespace {
  struct dtos {
    double x;
    bool imperial;

    dtos(double x, bool imperial) : x(x), imperial(imperial) {
      if (Math::isnan(x))
        THROW("Numerical error in GCode stream:  NaN, caused by a divide by "
              "zero or other math error.");

      if (Math::isinf(x))
        THROW("Numerical error in GCode stream: Infinite value");
    }


    string toString() const {return String(x, imperial ? 3 : 2);}
  };


  inline ostream &operator<<(ostream &stream, const dtos &d) {
    return stream << d.toString();
  }
}


void GCodeMachine::beginLine() {
#define DIGIT_CHARS        "1234567890"
#define LOWER_CHARS        "abcdefghijklmnopqrstuvwxyz"
#define UPPER_CHARS        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ALPHA_CHARS        LOWER_CHARS UPPER_CHARS
#define ALPHANUMERIC_CHARS ALPHA_CHARS DIGIT_CHARS
#define UNESCAPED ALPHANUMERIC_CHARS "-_.!~*/"

  const FileLocation &newLoc = getLocation().getStart();
  const string &filename = newLoc.getFilename();

  if (filename != location.getFilename()) {
    *stream << "(File: '" << URI::encode(filename, UNESCAPED) << "')\n";
    location.setFilename(filename);
  }

  if (newLoc.getLine() != location.getLine()) {
    *stream << 'N' << newLoc.getLine() << ' ';
    location.setLine(newLoc.getLine());
  }
}


void GCodeMachine::start() {
  *stream << (units == Units::METRIC ? "G21" : "G20") << "\n";
  // TODO set other GCode state
}


void GCodeMachine::end() {
  // Probably should be part of the machine description.
  *stream << "M2\n";
}


void GCodeMachine::setFeed(double feed) {
  double oldFeed = getFeed();
  MachineAdapter::setFeed(feed);

  if (feed != oldFeed) {
    beginLine();
    *stream << "F" << dtos(feed, false) << '\n';
  }
}


void GCodeMachine::setFeedMode(feed_mode_t mode) {
  feed_mode_t oldMode = getFeedMode();
  MachineAdapter::setFeedMode(mode);

  if (oldMode != mode)
    switch (mode) {
    case INVERSE_TIME:         *stream << "G93\n"; break;
    case UNITS_PER_MINUTE:     *stream << "G94\n"; break;
    case UNITS_PER_REVOLUTION: *stream << "G95\n"; break;
    default: THROW("Feed mode must be one of INVERSE_TIME, "
                   "UNITS_PER_MIN or UNITS_PER_REV");
    }
}


void GCodeMachine::setSpeed(double speed) {
  double oldSpeed = getSpeed();
  MachineAdapter::setSpeed(speed);

  if (oldSpeed != speed) {
    beginLine();

    if (0 < speed) *stream << "M3 S" << dtos(speed, false) << '\n';
    else if (speed < 0) *stream << "M4 S" << dtos(-speed, false) << '\n';
    else *stream << "M5\n";
  }
}


void GCodeMachine::setSpinMode(spin_mode_t mode, double max) {
  double oldMax = 0;
  spin_mode_t oldMode = getSpinMode(&oldMax);

  MachineAdapter::setSpinMode(mode, max);

  if (oldMode != mode || (oldMax != max && mode == CONSTANT_SURFACE_SPEED)) {
    beginLine();

    switch (mode) {
    case REVOLUTIONS_PER_MINUTE: *stream << "G97\n"; break;
    case CONSTANT_SURFACE_SPEED:
      *stream << "G96 S" << getSpeed(); // Must output speed with G96
      if (max) *stream << " D" << dtos(max, false);
      *stream << '\n';
      break;
    }
  }
}


void GCodeMachine::changeTool(unsigned tool) {
  unsigned currentTool = get(TOOL_NUMBER);

  MachineAdapter::changeTool(tool);

  if (tool != currentTool) {
    beginLine();
    *stream << "M6 T" << tool << '\n';
  }
}


void GCodeMachine::wait(port_t port, bool active, double timeout) {
  // TODO
}


void GCodeMachine::seek(port_t port, bool active, bool error) {
  // TODO
}


void GCodeMachine::output(port_t port, double value) {
  if (value)
    switch (port) {
    case FLOOD: *stream << "M7\n"; return;
    case MIST:  *stream << "M8\n"; return;
    default: break;
    }

  else
    switch (port) {
    case FLOOD: *stream << "M9\n"; return; // M9 turns off both mist and flood
    case MIST: return;
    default: break;
    }

  return MachineAdapter::output(port, value);
}


void GCodeMachine::dwell(double seconds) {
  beginLine();
  *stream << "G4 P" << dtos(seconds, false) << '\n';
  MachineAdapter::dwell(seconds);
}


bool is_near(double x, double y) {
  return y - 2 * numeric_limits<double>::epsilon() <= x &&
    x <= y + numeric_limits<double>::epsilon() * 2;
}


void GCodeMachine::move(const Axes &axes, bool rapid) {
  bool first = true;
  bool imperial = units == Units::IMPERIAL;
  Axes position = getPosition();

  for (const char *axis = Axes::AXES; *axis; axis++)
    if (!is_near(position.get(*axis), axes.get(*axis))) {
      string last = dtos(position.get(*axis), imperial).toString();
      string value = dtos(axes.get(*axis), imperial).toString();

      if (last == value) continue;

      if (first) {
        beginLine();
        *stream << 'G' << (rapid ? '0' : '1');
        first = false;
      }

      *stream << ' ' << *axis << value;
    }

  if (!first) {
    *stream << '\n';
    MachineAdapter::move(axes, rapid);
  }
}


void GCodeMachine::pause(bool optional) {
  beginLine();
  *stream << (optional ? "M1" : "M0") << '\n';
  MachineAdapter::pause(optional);
}


void GCodeMachine::comment(const string &s) const {
  vector<string> lines;
  String::tokenize(s, lines, "\n\r", true);

  for (unsigned i = 0; i < lines.size(); i++)
    *stream << "(" << lines[i] << ")\n";
}
