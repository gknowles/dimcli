# dim-cli

Making command line interfaces fun to write.

Main features:
- parses directly to c++ variables (or makes proxies for them)
- supports parsing to any type that is:
  - default constructable
  - assignable from std::string or has an istream extraction operator
- help page generation
- light weight

## Download
[build status]
[link to latest]

## Include in your project
- Make the library
- Copy the source files

## Documentation
Check out the [wiki](https://github.com/gknowles/dimcli/wiki), you'll be 
glad you did!

## Aspirational Roadmap
- callbacks
- help text and formatting options
- load from environment variables?
- support /name:value syntax
- composable subcommands
- access to unrecognized named options?
- tuple arguments

