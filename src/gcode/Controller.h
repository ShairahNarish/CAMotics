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

#pragma once

#include "Axes.h"
#include "Addresses.h"

#include <cbang/SmartPointer.h>
#include <cbang/LocationRange.h>

#include <string>


namespace GCode {
  class Code;
  class Entity;

  struct EndProgram {}; // Exception


  class Controller {
  public:
    virtual ~Controller() {}

    // Parameters
    virtual double get(address_t addr) const = 0;
    virtual void set(address_t addr, double value) = 0;
    virtual bool has(const std::string &name) const = 0;
    virtual double get(const std::string &name) const = 0;
    virtual void set(const std::string &name, double value) = 0;

    // Variables
    virtual void setVar(char c, double value) = 0;

    // Motion mode
    virtual unsigned getCurrentMotionMode() = 0;
    virtual void setCurrentMotionMode(unsigned mode) = 0;

    // Synchronize
    virtual bool isSynchronizing() const = 0;
    virtual void synchronize(const Axes &position) = 0;

    // State
    virtual void setLocation(const cb::LocationRange &location) = 0;
    virtual void setFeed(double feed) = 0;
    virtual void setSpeed(double speed) = 0;
    virtual void setTool(unsigned tool) = 0;

    // Execution
    virtual void newBlock() = 0;
    virtual bool execute(const Code &code, int vars) = 0;
  };
}
