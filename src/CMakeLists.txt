#
# sippak -- SIP command line utility.
# Copyright (C) 2018, Stas Kobzar <staskobzar@modulis.ca>
#
# This file is part of sippak.
#
# sippack is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# sippack is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with sippack.  If not, see <http://www.gnu.org/licenses/>.
#

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/include/sippak.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/sippak.h"
  @ONLY)

# PJPROJECT compile flags
pkg_check_modules (PJSIP REQUIRED libpjproject>=2.7.1)

add_subdirectory (mod)
add_subdirectory (app)

add_executable (${PROJECT_NAME} main.c
  $<TARGET_OBJECTS:mod>
  $<TARGET_OBJECTS:app>
  )
target_link_libraries (${PROJECT_NAME} ${PJSIP_LIBRARIES} resolv ${EXTRA_LIBS})

install (TARGETS ${PROJECT_NAME} DESTINATION bin)

