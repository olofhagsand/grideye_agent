# grideye_agent
Cloud monitoring agent. 

Grideye_agent is part of grideye cloud monitoring software. Grideye consists of a controller, a dashboard, and agents. This is the agent part.  

Some documentation about grideye architecture can be found at
     http://grideye.nordu.net/arch

The grideye agent can be run in a process, in a VM or in a
container. It communicates with a grideye controller. 

When started, a grideye_agent typically makes a 'callhome' to an existing
grideye controller.

A typical installation is as follows:

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
    cd clixon; configure; make; sudo make install; sudo make install-include

Grideye_agent contains an open plugin interface.  Several plugins
are included in this release. Other authors have (and can) contribute
to the plugins. The plugin interface is documented in
plugins/grideye_plugin_v1.h.

Grideye_agent is covered by GPLv3, _except_ the plugins that are
covered by copyright and licenses in their source code header (if any).

I can be found at olof@hagsand.se.


