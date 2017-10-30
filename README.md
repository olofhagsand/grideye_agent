# Grideye_agent
Cloud monitoring agent. 

## Table of contents

  * [Starting](#starting)
  * [Installation](#installation)
  * [Plugins](#plugins)
  * [Licenses](#licenses)
  * [Contact](#contact)

Grideye_agent is part of grideye cloud monitoring software. Grideye consists of a controller, a dashboard, and agents. This is the agent part.  

Some documentation about grideye architecture can be found at
     http://grideye.nordu.net/arch

The grideye agent can be run in a process, in a VM or in a
container. It communicates with a grideye controller. 

## 1. Starting

When started, a grideye_agent typically makes a 'callhome' to an existing
grideye controller.

This is an example of how to start grideye_agent:

    ./grideye_agent -F -u <url> -I <id> -N <name>

where
- 'url' is the URL of the grideye server / controller
- 'id' is the UUID of the user as gievn in the controller.
- 'name' is the name of the agent as it will appear in grideye plots.

As an alternative grideye_agent can be run as a docker container.

## 2. Installation

A typical installation is as follows:

    > configure	       	        # Configure grideye_agent
    > make                      # Compile
    > sudo make install         # Install agent and plugins

The source builds one main program: grideye_agent. An
example startup-script is available in util/grideye_agent.

Grideye_agent requires [CLIgen](http://www.cligen.se) and [CLIXON](http://www.clicon.org) for building. To build and install CLIgen and CLIXON:

    git clone https://github.com/olofhagsand/cligen.git
    cd cligen; configure; make; sudo make install
    git clone https://github.com/clicon/clixon.git
       cd clixon; 
       configure --without-restconf --without-keyvalue; 
       make; 
       sudo make install; 
       sudo make install-include

## 3. Plugins

Grideye_agent contains an open plugin interface.  Several plugins
are included in this release. Other authors have (and can) contribute
to the plugins. 

The following steps illustrates how to add an example plugin to
grideye_agent and how to wrap it to produce input to grideye. The
example plugin is a Nagios plugin: 'check_http' plugin, see
[https://www.monitoring-plugins.org/doc/man/check_http.html]. There
are lots of Nagios plugins making it an easy way to extend Grideye.

### 3.1 The API

A grideye plugin is a dynamically loaded plugin written in C. You need
to define a couple of functions, compile it, place it in a directory,
and then start grideye_agent.

First, a grideye plugin must have the function:
grideye_plugin_init_v1() which returns an API table containing functions pointers. Later versions of this API willprobably appear.

The API table is as follows (all of the functions are optional):

Symbol | Type | Mandatory |Description
--- | --- | --- | ---
gp_version | Variable | Yes | Must be 1
gp_magic | Variable | Yes | Should be 0x3f687f03
gp_exit_fn | Function | No | An optional exit function
gp_test_fn | Function | Yes | The actual test function with input and output parameters.
gp_file_fn | Function | No | Predefined files and devices may be used in tests.
gp_input | Variable | Yes | Input parameter requested
gp_output | Variable | Yes | Format of output. Only "xml" supported

### 3.2 Identifying the input - parameters

check_http has lots of parameters. You can hardcode most, or leave as
defaults.

```
   check_http -H www.youtube.com -S
   HTTP OK: HTTP/1.1 200 OK - 495051 bytes in 1.751 second response time |time=1.751042s;;;0.000000 size=495051B;;;0
```

In this example, we have chosen the 'host' parameter as dynamically
configurable, which means we can dynamically change it in Grideye testcases.

This means we define 'gp_input=host' and gp_test_fn is called with 'host' as input parameter, example:

```
   gp_test_fn("www.youtube.com", ...)
```

### 3.3 Identifying the output - metrics

check_http has several outputs, such as the status (200 OK), size and
latency. The test function is written so that it outputs an XML string
such as:

```
   <httptime>1.751042</httptime><httpsize>495051</httpsize><httpstatus>200 OK</httpstatus>
```

### 3.4 Defining metrics in YANG

The three 'metrics' (time, size, status) need to be defined in the
Grideye YANG model as well, so that they can be handled by the grideye
controller.  These entries are added to the grideye YANG model as follows:

```
   module grideye-result {
      ...
      grouping metrics{
         ...
         leaf httpstatus{
            description "HTTP status code";
            type string;
	 }
         leaf httptime{
            description "HTTP request time";
            type int32;
            units ms;
         }
         leaf httpsize{
            description "HTTP response size";
            type int32;
            units bytes;
         }
      }
   }
```

Thereafter, the grideye controller needs to be restarted. Note that
there is already a wide variety of metrics available and you can most
likely use already existing.

### 3.5 Writing the test function

It is straightforward to run the test: you need to spawn the Nagios
plugin and the parse the data. In C, this is a little painful, but
with help of a help function, this is (a simplified way) to write
it. The full code can be found in [plugins/grideye_http.c]:

```
   int
   http_test(int        host,
             char     **outstr)
   {
      if (http_fork(_PROGRAM, "-H", "www.youtube.com", "-S", buf, buflen) < 0)
         goto done;
      sscanf(buf, "%*s %*s %*s %s %s %*s %d %*s %*s %lf\n",
            code0, code1, &size, &time);
      if ((slen = snprintf(*outstr, slen,
                         "<httpstatus>%s %s</httpstatus>"
                         "<httptime>%d</httptime>"
                         "<httpsize>%d</httpsize>",
                         code0, code1,
                         (int)(time*1000),
                         size)) <= 0)
          goto done;
      return 0; 
   }
```
   
### 3.6 Local test run

Once the plugin source code has been written, it is possible to run
the test as stand-alone. This is useful for verifying, validating and
debugging the test:

```
   gcc -o grideye_http grideye_http.c
   ./grideye_http 
   <httpstatus>200 OK</httpstatus><httptime>1430</httptime><httpsize>491172</httpsize>
```

### 3.7 Full grideye run

When completed, the grideye plugin is compiled into a loadable module:
'grideye_http.so.1', installed under /usr/local/lib/grideye and the grideye_agent is restarted:

```
   sudo systemctl restart grideye_agent.service
```

And the new metrics appears in the grideye controller, for plotting, alarming and analysis.

## 4. Licenses

Grideye_agent is covered by GPLv3, _except_ the plugins that are
covered by copyright and licenses in their source code header (if any).

## 5. Contact

I can be found at olof@hagsand.se.