/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef TEST_H
#define TEST_H

#include "base/graph.h"
#include "base/histogram.h"
#include "base/integrate_gradients.h"

void testGraph();
void testDijkstra();
void testSPFA();
void testSPFA2();
void testDivergence(const QString& input_filename, const QString& output_filename);
void testIntegrate(const QString& input_filename, const QString& output_filename);

#endif // TEST_H
