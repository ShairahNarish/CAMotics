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

#include "Program.h"

using namespace std;
using namespace cb;
using namespace GCode;


void Program::process(Processor &processor) {
  for (const_iterator it = begin(); it != end(); it++) processor(*it);
}


void Program::print(ostream &stream) const {
  for (const_iterator it = begin(); it != end(); it++)
    stream << **it << '\n';
}


void Program::operator()(const SmartPointer<Block> &block) {
  if (empty()) getLocation() = block->getLocation();
  else getLocation().getEnd() = block->getLocation().getEnd();

  push_back(block);
}
