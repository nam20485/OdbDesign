# Tools Configuration

## CLI Tools

You have access to a powershell shell wehere you can run any manner of commands.
This also allows you to install more tools to expand your capabilities. You have access to package managers on the current OS. E.g. choco and winget on Windows. apt on Debian-based *nixes.
Specific tools already installed include:

* gh cli (github cli)
* gcloud (google cloud cli)
* firebase (firebase cli)
* docker (docker cli)
* node (node.js)
* npm (node package manager)
* python (python)

 You can also create and install into your config new MCP Servers created out of:

* any CLI executable (e.g. bash, pwsh, cmd.exe, powershell)
* any npm package

In powershell you can also create ps1 scripts and run them. You can also create and run batch files. You can also run python and node scripts.

You can also expand your capabilities by installing additional python, and node packages and pwoershell modules.

## MCP  

## MCP Per Se

You have access to MCP servers and tools. You are not limited to the existing installed MCP servers. If needed, you can search online MCP server registries and add them manually so that you can them use.

## .NET Manually Created Servers

There is a MCP server solution that is manually created. This solution is located at ./../MCPServers/MCPServers.sln.
This solution contains a number of MCP servers. You can manually add new tools by implementing them in .NET as a new project or new tool in an existin project.
Make sure that the MCPServers' MCP servers are registered with Copilot Chat and VS Code. After creating a new server make sure run or restart the server's service.
