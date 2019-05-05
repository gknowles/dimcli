<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

## Multiple Source Files
Options don't have to be defined all in one source file, separate source
files can each define options of interest to that file and get them populated
when the command line is processed.

When you instantiate Dim::Cli you're creating a handle to the globally shared
configuration. So multiple translation units can each create one and use it to
update the shared configuration.

The following example has a logMsg function in log.cpp with its own "-1"
option while main.cpp registers "--version":

~~~ cpp
// main.cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.versionOpt("1.0");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    // do stuff that might call logMsg...
    return EX_OK;
}
~~~

~~~ cpp
// log.cpp
static Dim::Cli cli;
static auto & failEarly = cli.opt<bool>("1").desc("Exit on first error");

void logMsg(string & msg) {
    cerr << msg << endl;
    if (*failEarly)
        exit(EX_SOFTWARE);
}
~~~

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  -1         Exit on first error

  --help     Show this message and exit.
  --version  Show version and exit.
~~~

When you want to put a bundle of stuff in a separate source file, such as a
[command](Subcommands) and its options, it can be convenient to group them
into a single static struct.
~~~ cpp
// somefile.cpp
static int myCmd(Dim::Cli & cli);

static struct CmdOpts {
    int option1;
    string option2;
    string option3;

    CmdOpts() {
        Cli cli;
        cli.command("my").action(myCmd).desc("What my command does.");
        cli.opt(&option1, "1 one", 1).desc("First option.");
        cli.opt(&option2, "2", "two").desc("Second option.");
        cli.opt(&option3, "three", "three").desc("Third option.");
    }
} s_opts;
~~~

Then in myCmd() and throughout the rest of somefile.cpp you can reference the
options as **s_opts.option1**, **s_opts.option2**, and **s_opts.option3**.

And the help text will be:
~~~ console
$ a.out my --help
usage: a.out my [OPTIONS]

What my command does.

Options:
  -1, --one  First option. (default: 1)
  -2         Second option. (default: two)
  --three    Third option. (default: three)

  --help     Show this message and exit.
~~~

#### Dim::CliLocal
Although it was created for testing you can also use Dim::CliLocal, a
completely self-contained parser, if you need to redefine options, have
results from multiple parses at once, or otherwise avoid the shared
configuration.
