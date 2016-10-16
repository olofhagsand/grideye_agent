/*
  Copyright (C) 2015-2016 Olof Hagsand

  This file is part of GRIDEYE.

  GRIDEYE is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  GRIDEYE is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GRIDEYE; see the file LICENSE.  If not, see
  <http://www.gnu.org/licenses/>.

  This include file defines the API of a GRIDEYE plugin.
  The grideye_agent dynamically links and loads any plugin that is found
  in the plugin directory (typically /usr/local/lib/grideye).

  The copyright of a plugin is the author's own.
  The author may set any license he/she wishes.
  The act of loading a plugin with grideye_agent does not affect the 
  copyright of the plugin, nor its license
  Likewise, the loading of a plugin does not affect the ownership or license
  of the grideye_agent which is stated at the top of thsi file.
*/


/* Version of grideye plugin. */
#define GRIDEYE_PLUGIN_VERSION 1

/* Version of grideye plugin. */
#define GRIDEYE_PLUGIN_MAGIC 0x3f687f03

/* Name of plugin init function (must be called this) */
#define PLUGIN_INIT_FN_V1 "grideye_plugin_init_v1"

/* Type of plugin init function */
typedef void * (grideye_plugin_init_t)(int version);

/* Type of plugin exit function */
typedef int (grideye_plugin_exit_t)(void);

/* Type of plugin file function */
typedef int (grideye_plugin_file_t)(const char *writefile, 
				    const char *largefile,
				    const char *device);

/* Type of plugin test function */
typedef int (grideye_plugin_test_t)(int inparam, char **outstr);

/* grideye agent plugin init struct for the api */
struct grideye_plugin_api_v1{
    int                    gp_version;
    int                    gp_magic;
    grideye_plugin_exit_t *gp_exit_fn;
    grideye_plugin_test_t *gp_test_fn;
    grideye_plugin_file_t *gp_file_fn;
    char                  *gp_input_param;
    char                  *gp_output_format; /* only xml allowed */
};



