<!--
Copyright Glen Knowles 2016 - 2019.
Distributed under the Boost Software License, Version 1.0.
-->

# Overview
C++ command line parser toolkit for kids of all ages.

- GNU style command lines (-o, --output=FILE, etc.)
- parses directly to any supplied (or implicitly created) variable that is:
    - default constructible
    - copyable
    - assignable from string, has an istream extraction operator, or has a
      specialization of Cli::fromString&lt;T>()
- help generation
- option definitions can be scattered across multiple files
- git style subcommands
- response files (requires `<filesystem>` support)
- works whether exceptions and RTTI are enabled or disabled
- distributed under the Boost Software License, Version 1.0.

How does it feel?

~~~ cpp
#include "dimcli/cli.h"
#include <iostream>
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & count = cli.opt<int>("c n count", 1).desc("times to say hello");
    auto & name = cli.opt<string>("name", "Unknown").desc("who to greet");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name)
        cout << "Greeting the unknown." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return 0;
}
~~~
What it does when run:

~~~ console
$ a.out -x
Error: Unknown option: -x

$ a.out --help
Usage: a.out [OPTIONS]

Options:
  -c, -n, --count=INTEGER  times to say hello (default: 1)
  --name STRING            who to greet (default: Unknown)

  --help                   Show this message and exit.

$ a.out --count=3
Greeting the unknown.
Hello Unknown!
Hello Unknown!
Hello Unknown!

$ a.out --name John
Hello John!
~~~


# Basics

## Basic Usage
After inspecting args cli.parse() returns false if it thinks the program
should exit, in which case cli.exitCode() is either EX_OK (0) or EX_USAGE (64)
for early exit (like --help) or bad arguments respectively. Otherwise the
command line was valid, arguments have been parsed, and cli.exitCode() is
EX_OK.

~~~ cpp
#include "dimcli/cli.h"
#include <iostream>
#include <sysexits.h> // if you want the unix exit code macros (EX_*)
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the apple have a worm? No!";
    return EX_OK;
}
~~~

And what it looks like:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help    Show this message and exit.

$ a.out
Does the apple have a worm? No!
~~~

The EX_* constants (along with standard values) are in `<sysexits.h>` on most
unixes, although it may not be in any standard. Equivalent enum values
Dim&#58;:kExitOk (0) and Dim::kExitUsage (64) are defined, which can be useful
on Windows where `<sysexits.h>` doesn't exist.


## Options
Dim::Cli is used by declaring options to receive arguments. Either via
pointer to a predefined external variable or by implicitly creating the
variable when the option is declared.

Use cli.opt&lt;T>(names, defaultValue) to link positional or named arguments to
an option. It returns a proxy object that can be used like a pointer (* and ->)
to access the value.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & fruit = cli.opt<string>("fruit", "apple");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << *fruit << " have a worm? No!";
    return EX_OK;
}
~~~

And what you get:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --fruit=STRING  (default: apple)

  --help          Show this message and exit.

$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out --fruit orange
Does the orange have a worm? No!
~~~

Add a description and change the value's name in the description:

~~~ cpp
auto & fruit = cli.opt<string>("fruit", "apple")
    .desc("type of fruit")
    .valueDesc("FRUIT");
~~~
And you get:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --fruit=FRUIT  type of fruit (default: apple)

  --help         Show this message and exit.
~~~


## External Variables
In addition to using the option proxies you can bind options directly to
predefined variables. This can be used to set a global flag, or populate a
struct that you access later.

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    bool worm;
    Dim::Cli cli;
    cli.opt(&worm, "w worm").desc("make it icky");
    auto & fruit = cli.opt<string>("fruit", "apple").desc("type of fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << *fruit << " have a worm? "
        << worm ? "Yes :(" : "No!";
    return EX_OK;
}
~~~
And what it looks like:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --fruit=STRING  type of fruit (default: apple)
  -w, --worm      make it icky

  --help          Show this message and exit.

$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out -w
Does the apple have a worm? Yes :(
~~~

You can also point multiple "options" at the same variable, as is common with
[feature switches](#feature-switches).


## Option Names
Names are passed in as a space separated list where the individual names look
like these:

| Type of name                        | Example      |
|-------------------------------------|--------------|
| short name (single character)       | f            |
| long name (more than one character) | file         |
| optional positional                 | \[file name] |
| required positional                 | &lt;file>    |

Names for positionals (inside angled or square brackets) may contain spaces,
and all names may have modifier flags:

| Flag | Type   | Description                                                     |
| :--: |--------|-----------------------------------------------------------------|
| !    | prefix | for boolean values, when setting the value it is first inverted |
| ?    | prefix | for non-boolean named options, makes the value [optional](#optional-values) |
| .    | suffix | for boolean values with long names, suppresses the implicit "no-" version |

By default, long names for boolean values get a second "no-" version implicitly
created for them.

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.opt<string>("a apple [apple]").desc("apples are red");
    cli.opt<bool>("!o orange apricot.").desc("oranges are orange");
    cli.opt<string>("<pear>").desc("pears are yellow");
    cli.parse(cerr, argc, argv);
    return EX_OK;
}
~~~
Ends up looking like this (note: required positionals are **always** placed
before any optional ones):

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] pear [apple]
  pear      pears are yellow
  apple     apples are red

Options:
  -a, --apple=STRING          apples are red
  --apricot, --orange / -o, --no-orange
                              oranges are orange

  --help                      Show this message and exit.
~~~

When named options are added they replace any previous rule with the same
name, therefore this option declares '-n' an inverted bool:

~~~ cpp
cli.opt<bool>("n !n");
~~~
But with these it becomes '-n STRING', a string:

~~~ cpp
cli.opt<bool>("n !n");
cli.opt<string>("n");
~~~


## Positional Arguments
A few things to keep in mind about positional arguments:

- Positional arguments are mapped by the order they are added, except that
  required ones appear before optional ones.
- If there are multiple vector positionals with unlimited (nargs = -1) arity
  all but the first will be treated as if they had nargs = 1. Or to put it
  another way, the first unlimited positional is greedy.
- If the unlimited one is required it will prevent any optional positionals
  from getting populated, since it eats up all the arguments before they get
  a turn.


## Flag Options
Many options are flags with no associated value, they just set an option
to a predefined value. This is the default when you create a option of type
bool. Normally flags set the option to true, but this can be changed in two
ways:

- make it an inverted bool, which will set it to false
  - explicitly using the "!" modifier
  - define a long name and use the implicitly created "no-" prefix version
- use opt.flagValue() to set the value, see
  [feature switches](#feature-switches).

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & shout = cli.opt<bool>("shout !whisper").desc("I can't hear you!");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    string prog = argv[0];
    if (*shout) {
        auto & f = use_facet<ctype<char>>(cout.getloc());
        f.toupper(prog.data(), prog.data() + prog.size());
        prog += "!!!!111";
    }
    cout << "I am " << prog;
    return EX_OK;
}
~~~
What you see:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --shout, --no-whisper / --no-shout, --whisper
            I can't hear you!

  --help    Show this message and exit.

$ a.out
I am a.out
$ a.out --shout
I am A.OUT!!!!111
$ a.out --no-whisper
I am A.OUT!!!!111
~~~


## Vector Options
Allows for an unlimited (or specific) number of values to be returned in a
vector. Vector options are declared using cli.optVec() which binds to a
std::vector&lt;T>.

Example:

~~~ cpp
// printing a comma separated list is annoying...
template<typename T>
ostream & operator<< (ostream & os, vector<T> const & v) {
    auto i = v.begin(), e = v.end();
    if (i != e) {
        os << *i++;
        for (; i != e; ++i) os << ", " << *i;
    }
    return os;
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    // for oranges demonstrate using an external vector, and limit
    // the maximum number to 2.
    vector<string> oranges;
    cli.optVec(&oranges, "o orange", 2).desc("oranges");
    // for apples demonstrate just using the proxy object.
    auto & apples = cli.optVec<string>("[apple]").desc("red fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Comparing (" << *apples << ") and (" << *oranges << ").";
    return EX_OK;
}
~~~
View from the command line:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] [apple...]
  apple     red fruit

Options:
  -o, --orange=STRING  oranges (limit: 2)

  --help               Show this message and exit.

$ a.out -o mandarin -onavel "red delicious" honeycrisp
Comparing (red delicious, honeycrisp) and (mandarin, navel).
$ a.out -omandarin -onavel -ohamlin
Error: Too many '-o' values: hamlin
The maximum number of values is: 2
~~~

While the * and -> operators get you full access to the underlying vector,
size() and [] are also available directly on the OptVec&lt;T>. Which may
occasionally save a little bit of typing.

~~~ cpp
auto & apples = cli.optVec<string>("[apple]").desc("red fruit");
...
cout << "There were " << apples.size() << " apples." << endl;
if (apples)
    cout << "The first was " << apples[0] << endl;
~~~


## Life After Parsing
If you are using external variables you just access them directly after using
cli.parse() to populate them.

If you use the proxy object returned from cli.opt&lt;T>() you can dereference it
like a smart pointer to get at the value. In addition, you can test whether
it was explicitly set, find the argument name that populated it, and get the
position in argv\[] it came from.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & name = cli.opt<string>("n name", "Unknown");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name) {
        cout << "Using the unknown name." << endl;
    } else {
        cout << "Name selected using " << name.from()
            << " from argv[" << name.pos() << "]" << endl;
    }
    cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
~~~
What it does:

~~~ console
$ a.out
Using the unknown name.
Hello Unknown!
$ a.out -n John
Name selected using -n
Hello John!
$ a.out --name Mary
Name selected using --name from argv[2]
Hello Mary!
~~~

If you want a little more control over error output you can use the two
argument version of cli.parse() and then get the results with cli.printError()
or manually using cli.exitCode(), cli.errMsg(), and cli.errDetail().

~~~ cpp
if (!cli.parse(argc, argv))
    return cli.printError(cerr);
~~~

Because (unless you use CliLocal) there is a single program wide command line
context, you can make an error handler that doesn't have to be passed the
results.

~~~ cpp
void failed() {
    Dim::Cli cli;
    cli.printError(cerr);
    exit(cli.exitCode());
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    if (!cli.parse(argc, argv))
        failed();
    ...
    return EX_OK;
}
~~~

# Advanced

## Special Arguments

| Value        | Description                                                      |
|--------------|------------------------------------------------------------------|
| "-"          | Passed in as a positional argument.                              |
| "--"         | Thrown away, but makes all remaining arguments positional        |
| "@&lt;file>" | [Response file](#response-files) containing additional arguments |


## Optional Values
You use the '?' [flag](#option-names) on an argument name to indicate that
its value is optional. Only non-booleans can have optional values, booleans
are evaluated just on their presence or absence and don't otherwise have
values.

For a user to set a value on the command line when it is optional the value
must be connected (no space) to the argument name, otherwise it is interpreted
as not present and the arguments implicit value is used instead. If the name
is not present at all the variable is set to the default given in the
cli.opt&lt;T>() call.

By default the implicit value is T{}, but can be changed using
opt.implicitValue().

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & v1 = cli.opt<string>("?o ?optional", "default");
    auto & v2 = cli.opt<string>("?i ?with-implicit", "default");
    v2.implicitValue("implicit");
    auto & p = cli.opt<string>("[positional]", "default");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "v1 = " << *v1 << ", v2 = " << *v2 << ", p = " << *p;
    return EX_OK;
}
~~~
What happens:

~~~ console
$ a.out
v1 = default, v2 = default, p = default
$ a.out -oone -i two
v1 = one, v2 = implicit, p = two
$ a.out -o one -itwo
v1 =, v2 = two, p = one
$ a.out --optional=one --with-implicit two
v1 = one, v2 = implicit, p = two
$ a.out --optional one --with-implicit=two
v1 =, v2 = two, p = one
~~~


## Before Actions
It's unusual to want a before action. They operate on the entire argument
list, after environment variable and response file expansion, but before any
individual arguments are parsed. The before action should:

- Inspect and possibly modify the raw arguments. The args are guaranteed to
  start out valid, but be careful that it still starts with a program name
  in arg0 when you're done.
- Call cli.badUsage() with an error message for problems.
- Return false if the program should stop, otherwise true.

There can be any number of before actions, they are executed in the order
they are added.

Let's test for empty command lines and add "--help" to them. But first, our
"before" program:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & val = cli.opt<string>("<value>").desc("It's required!");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "The value: " << *val;
    return EX_OK;
}
~~~

And it's output:
~~~ console
$ a.out 99
The value: 99
$ a.out --help
usage: a.out [OPTIONS]
  value     It's required!

Options:
  --help    Show this message and exit.
$ a.out
Error: Missing argument: value
~~~

Now add the before action:
~~~ cpp
cli.before([](Cli &, vector<string> & args) {
    if (args.size() == 1) // it's just the program name?
        args.push_back("--help");
    return true;
});
~~~

And missing arguments are a thing of the past...
~~~ console
$ a.out
usage: a.out [OPTIONS]
  value     It's required!

Options:
  --help    Show this message and exit.
~~~

That isn't too complicated, but for this case cli.helpNoArgs() is available
to do the same thing.


## Parse Actions
Sometimes, you want an argument to completely change the execution flow. For
instance, to provide more detailed errors about badly formatted arguments. Or
to make "--version" print some crazy ASCII artwork and exit the program (for
a non-crazy --version use [opt.versionOpt()](#version-option)).

Parsing actions are attached to options and get invoked when a value becomes
available for it. Any std::function compatible object that accepts references
to cli, opt, and string as parameters can be used. The function should:

- Parse the source string and use the result to set the option value (or
  push back the additional value for vector arguments).
- Call cli.badUsage() with an error message if there's a problem.
- Return false if the program should stop, otherwise true. You may want to
  stop due to error or just to early out like "--version" and "--help".

Other things to keep in mind:

- Options only have one parse action, changing it *replaces* the default.
- You can use opt.from() and opt.pos() to get the argument name that the value
  was attached to on the command line and its position in argv\[].
- For bool options the source value string will always be either "0" or "1".

Here's an action that multiples multiple values together:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & sum = cli.opt<int>("n number", 1)
        .desc("numbers to multiply")
        .parse([](auto & cli, auto & opt, string const & val) {
            int tmp = *opt; // save the old value
            if (!opt.parseValue(val)) // parse the new value into opt
                return cli.badUsage(opt, val);
            *opt *= tmp; // multiply old and new together
            return true;
        });
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "The product is: " << *sum << endl;
    return EX_OK;
}
~~~

Let's do some math!
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -n, --number=NUM  numbers to multiply (default: 1)

  --help            Show this message and exit.

$ a.out
The product is: 1
$ a.out -n3 -n2
The product is: 6
$ a.out -nx
Error: Invalid '-n' value: x
~~~


## Check Actions
Check actions run for each value that is successfully parsed and are a good
place for additional work. For example, opt.range() and opt.clamp() are
implemented as check actions. Just like parse actions the callback is any
std::function compatible object that accepts references to cli, opt, and
string as parameters and returns bool.

An option can have any number of check actions and they are called in the
order they were added.

The function should:
- Check the options new value. Beware that options are process in the order
  they appear on the command line, so comparing with another option is
  usually better done in an [after action](#after-actions).
- Call cli.badUsage() with an error message if there's a problem.
- Return false if the program should stop, otherwise true to let processing
  continue.

The opt is fully populated, so *opt, opt.from(), etc are all available.

Sample check action that rounds up to an even number of socks:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & socks = cli.opt<int>("socks")
        .desc("Number of socks, rounded up to even number.")
        .check([](auto & cli, auto & opt, auto & val) {
            *opt += *opt % 2;
            return true;
        });
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << *socks << " socks";
    if (*socks) cout << ", where are the people?";
    cout << endl;
    return EX_OK;
}
~~~

Let's... wash some socks?
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -socks=NUM  Number of socks, rounded up to even number.

  --help      Show this message and exit.

$ a.out
0 socks
$ a.out --socks 3
4 socks, where are the people?
~~~


## After Actions
After actions run after all arguments have been parsed. For example,
opt.prompt() and opt.require() are both implemented as after actions. Any
number of after actions can be added and will, for every (not just the
ones referenced by the command line!) registered option, be called in the
order they're added. They are called with the three parameters, like other
option actions, that are references to cli, opt, and the value string
respectively. However the value string is always empty(), so any information
about the value must come from the opt reference.

When using subcommands, only the after actions bound to the top level or the
selected command are executed. After actions on the options of all other
commands are, like the options themselves, ignored.

The function should:
- Do something interesting.
- Call cli.badUsage() and return false on error.
- Return true if processing should continue.

Action to make sure the high is not less than the low:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & low = cli.opt<int>("l").desc("Low value.");
    auto & high = cli.opt<int>("h")
        .desc("High value, must be greater than the low.")
        .after([&](auto & cli, auto & opt, auto &) {
            return (*opt >= *low)
                || cli.badUsage("High must not be less than the low.");
        });
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Range is from " << *low << " to " << *high << endl;
    return EX_OK;
}
~~~

Set the range:
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -h=NUM    High value, must be greater than the low.
  -l=NUM    Low value.

  --help    Show this message and exit.

$ a.out
Range is from 0 to 0
$ a.out -l1
High must not be less than the low.
$ a.out -h5 -l2
Range is from 2 to 5
~~~


## Subcommands
Git style subcommands are created by either cli.command("cmd"), which changes
the cli objects context to the command, or with opt.command("cmd"), which
changes the command that option is for. Once the cli object context has been
changed it can than be used to add (description, footer, options, etc) to the
command. Exactly the same as when working with a simple command line. If you
pass an empty string to cli.command() or opt.command() it represents the top
level processing that takes place before a command has been found.

Options are processed on the top level up to the first positional. The first
positional is the command, and the rest of the arguments are processed in the
context of that command. Since the top level doesn't process positionals when
commands are present, it will assert in debug builds and ignore them in
release if positionals are defined.
~~~ cpp
static auto & yell = Dim::Cli().opt<bool>("yell").desc("Say it loud.");
static auto & color = Dim::Cli().opt<string>("color", "red")
    .command("apple")
    .desc("Change color of the apple.");

int apple(Dim::Cli & cli) {
    cout << "It's a " << *color << " apple" << (*yell ? "!!!" : ".");
    return EX_OK;
}

int orange(Dim::Cli & cli) {
    cout << "It's an orange" << (*yell ? "!!!" : ".");
    return EX_OK;
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.command("apple").desc("Show apple. No other fruit.").action(apple);
    cli.command("orange").desc("Show orange.").action(orange);
    cli.exec(cerr, argc, argv);
    return cli.exitCode();
}
~~~

The same thing could also be done with external variables:
~~~ cpp
static bool yell;
static string color;
...

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.opt(&yell, "yell").desc("Say it loud.");
    cli.opt(&color, "color", "red").command("apple")
        .desc("Change color of the apple.");
    ...
~~~

Or if there's some additional argument checks or setup you need to do, the
exec() call can be separate from parse():
~~~ cpp
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    // any additional validation...
    cli.exec(cerr);
    return cli.exitCode();
~~~

The end result at the console:
~~~ console
$ a.out
Error: No command given.
$ a.out --help
usage: a.out [OPTIONS] command [args...]

Commands:
  apple     Show apple.
  orange    Show orange.

Options:
  --yell    Say it loud.

  --help    Show this message and exit.

$ a.out apple
It's a red apple.
$ a.out apple --color=yellow
It's a yellow apple.
$ a.out orange
It's an orange.
$ a.out --yell orange
It's an orange!!!
~~~

In the commands list, only the first sentence of cli.desc() (up to the first
'.', '!', or '?' that's followed by a space) is shown, but in command specific
pages you see the whole thing:

~~~ console
$ a.out apple --help
usage: a.out apple [OPTIONS]

Show apple. No other fruit.

Options:
  --color=STRING  Change color of the apple. (default: red)

  --help          Show this message and exit.
~~~


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
[command](#subcommands) and its options, it can be convenient to group them
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
You can also use Dim::CliLocal, a completely self-contained parser, if you
need to redefine options, have results from multiple parses at once, or
otherwise avoid the shared configuration.


## Response Files
A response file is a collection of frequently used or generated arguments
saved as text, often with a ".rsp" extension, that is substituted into the
command line when referenced.

What you write:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & words = cli.optVec<string>("[words]").desc("Things you say.");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Words:";
    for (auto & w : words)
        cout << " " << w;
    return EX_OK;
}
~~~
What happens later:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] [words...]
  words     Things you say.

Options:
  --help    Show this message and exit.

$ a.out a b
Words: a b
$ echo c >one.rsp
$ a.out a b @one.rsp d
Words: a b c d
~~~
Response files can be used multiple times and the arguments in them can be
broken into multiple lines:

~~~ console
$ echo d >one.rsp
$ echo e >>one.rsp
$ a.out x @one.rsp y @one.rsp
Words: x d e y d e
~~~
Response files also can be nested, when a response file contains a reference
to another response file the path is relative to the parent response file,
not to the working directory.

~~~ console
$ md rsp & cd rsp
$ echo one @more.rsp >one.rsp
$ echo two three >more.rsp
$ cd ..
$ a.out @rsp/one.rsp
Words: one two three
~~~

Recursive response files will fail, don't worry!
~~~ console
$ echo "@one.rsp" >one.rsp
$ a.out @one.rsp
Error: Recursive response file: one.rsp
~~~

While generally useful response file processing can be disabled via
cli.responseFiles(false).


## Environment Variable
You can specify an environment variable that will have its contents
prepended to the command line. This happens before response file expansion
and any before actions.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & words = cli.optVec<string>("[words]");
    cli.envOpts("AOUT_OPTS");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Words:";
    for (auto && word : *words)
        cout << " '" << word << "'";
    return EX_OK;
}
~~~
The same can also be done manually, as shown below. This is a good starting
point if you need something slightly different:

~~~ cpp
vector<string> args = cli.toArgv(argc, argv);
if (char const * eopts = getenv("AOUT_OPTS")) {
    vector<string> eargs = cli.toArgv(eopts);
    // Insert the environment args after arg0 (program name) but before
    // the rest of the command line.
    args.insert(args.begin() + 1, eargs.begin(), eargs.end());
}
if (!cli.parse(cerr, args))
    return cli.exitCode();
~~~

Or as a before action (after response file expansion):
~~~ cpp
cli.before([](Cli &, vector<string> & args) {
    if (char const * eopts = getenv("AOUT_OPTS")) {
        vector<string> eargs = cli.toArgv(eopts);
        args.insert(args.begin() + 1, eargs.begin(), eargs.end());
    }
});
if (!cli.parse(cerr, args))
    return cli.exitCode();
~~~

How this works:

~~~ console
$ export AOUT_OPTS=
$ a.out c d
Words: 'c' 'd'
$ export AOUT_OPTS=a b
$ a.out 'c' 'd'
Words: 'a' 'b' 'c' 'd'
~~~


## Keep It Quiet
For some applications, such as Windows services, it's important not to
interact with the console. Simple steps to avoid cli.parse() doing console IO:

1. Don't use things (such as opt.prompt()) that explicitly ask for IO.
2. Add your own "help" argument to override the default, you can still turn
around and call cli.printHelp(ostream&) if desired.
3. Use the two argument version of cli.parse() and get the error message from
cli.errMsg() and cli.errDetail() if it fails.


# Options and Modifiers

## Version Option
Use cli.versionOpt() to add simple --version processing.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.versionOpt("1.0");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Hello world!" << endl;
    return EX_OK;
}
~~~

Is version 1.0 ready to ship?
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help     Show this message and exit.
  --version  Show version and exit.

$ a.out --version
a.out version 1.0
$ a.out
Hello world!
~~~


## Help Option
You can modify the implicitly create --help option. Use cli.helpOpt() to get
a reference and then go to town. The most likely thing would be to change the
description or option group, but since you get back an Opt&lt;T> you can use any
of the standard functions.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.helpOpt();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return EX_OK;
}
~~~

And when run...
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help    Show this message and exit.
~~~

It can be modified like any other bool option.
~~~ cpp
cli.helpOpt().desc("What you see is what you get.");
~~~
~~~ cpp
auto & help = cli.helpOpt();
help.desc("What you see is what you get.");
~~~

Either of which gets you this:
~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help    What you see is what you get.
~~~

Another related command is cli.helpNoArgs(), which internally adds "--help" to
otherwise empty command lines.
~~~ cpp
cli.helpNoArgs();
cli.helpOpt().desc("What you see is what you get.");
~~~

Now all there is, is help:
~~~ console
$ a.out
usage: a.out [OPTIONS]

Options:
  --help    What you see is what you get.
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --help    What you see is what you get.
~~~


## Feature Switches
Using flag options, feature switches are implemented by creating multiple
options that reference the same external variable and marking them as flag
values.

To set the default, pass in a value of true to the flagValue() function of
the option that should be the default.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    string fruit;
    // "~" is the default option group for --help, --version, etc. Give
    // it a title so it doesn't look like more fruit.
    cli.group("~").title("Other options");
    cli.group("Type of fruit");
    cli.opt(&fruit, "o orange", "orange").desc("oranges").flagValue();
    cli.opt(&fruit, "a", "apple").desc("red fruit").flagValue(true);
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << fruit << " have a worm? No!";
    return EX_OK;
}
~~~
Which looks like:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Type of fruit:
  -a            red fruit (default)
  -o, --orange  oranges

Other options:
  --help        Show this message and exit.

$ a.out
Does the apple have a worm? No!
$ a.out -o
Does the orange have a worm? No!
~~~
You can use an inaccessible option (empty string for the keys) that doesn't
show up in the interface (or the help text) to set an explicit default.

~~~ cpp
cli.opt(&fruit, "o orange", "orange").desc("oranges").flagValue();
cli.opt(&fruit, "a", "apple").desc("red fruit").flagValue();
cli.opt(&fruit, "", "fruit").flagValue(true);
~~~
Now instead of an apple there's a generic fruit default.

~~~ console
$ a.out
Does the fruit have a worm? No!
~~~


## Choice
Sometimes you want an option to have a fixed set of possible values, such as
for an enum. You use opt.choice() to add legal choices, one at a time, to an
option.

Choices are similar to [feature switches](#feature-switches) but instead of
multiple boolean options populating a single variable it is a single
non-boolean option setting its variable to one of multiple values.

~~~ cpp
enum class State { go, wait, stop };

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & state = cli.opt<State>("streetlight", State::wait)
        .desc("Color of street light.").valueDesc("COLOR")
        .choice(State::go, "green", "Means go!")
        .choice(State::wait, "yellow", "Means wait, even if you're late.")
        .choice(State::stop, "red", "Means stop.");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    switch (*state) {
        case State::stop: cout << "STOP!"; break;
        case State::go: cout << "Go!"; break;
        case State::wait: cout << "Wait"; break;
    }
    return EX_OK;
}
~~~

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --streetlight=COLOR  Color of street light.
      green   Means go!
      yellow  Means wait, even if you're late. (default)
      red     Means stop.

  --help               Show this message and exit.

$ a.out
Wait
$ a.out --streetlight
Error: Option requires value: --streetlight
$ a.out --streetlight=purple
Error: Invalid "--streetlight" value: purple
Error: Must be "green", "red", or "yellow"
$ a.out --streetlight=green
Go!
~~~


## Require
A simple way to make sure an option is specified is to mark it required with
opt.require(). This adds an after action that fails if no explicit value was
set for the option.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & file = cli.opt<string>("file f").require();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Selected file: " << *file << endl;
    return EX_OK;
}
~~~

What you get:
~~~ console
$ a.out
Error: No value given for --file
$ a.out -ffile.txt
Selected file: file.txt
~~~

The error message references the first name in the list so if you flip it
around...
~~~ cpp
auto & file = cli.opt<string>("f file").require();
~~~

... it will complain about '-f' instead of '--file'.
~~~ console
$ a.out
Error: No value given for -f
~~~


## Range and Clamp
When you want to limit a value to be within a range (inclusive) you can use
opt.range() to error out or opt.clamp() to convert values outside the range to
be equal to the nearest of the two edges.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & count = cli.opt<int>("<count>").clamp(1, 10);
    auto & letter = cli.opt<char>("<letter>").range('a','z');
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << string(*count, *letter) << endl;
    return EX_OK;
}
~~~

~~~ console
$ a.out 1000 b
bbbbbbbbbb
$ a.out 1000 1
Error: Out of range 'letter' value: 1
Must be between 'a' and 'z'.
~~~


## Units of Measure
The opt.siUnits(), opt.timeUnits(), and opt.anyUnits() are implemented as
parser actions and provide a simple way to support unit suffixes on numerical
values. The value has the units are removed, is parsed as a double, multiplied
by the associated factor, rounded to an integer (unless the target is a
floating point type), converted back to a string, and then finally passed to
cli.fromString&lt;T>().

Flag              | Description
------------------|------------
fUnitBinaryPrefix | Only for opt.siUnits(), makes k,M,G,T,P factors of 1024 (just like ki,Mi,Gi,Ti,Pi), and excludes fractional unit prefixes (milli, micro, etc).
fUnitInsensitive  | Makes units case insensitive. For opt.siUnits(), unit prefixes are also case insensitive and fractional unit prefixes are excluded. So 'M' and 'm' are both mega.
fUnitRequire      | Values without units are rejected, even if they have unit prefixes (k,M,G,etc).

### SI Units
SI units are considered to be anything that uses the SI prefixes. The
supported prefixes range from 1e+15 to 1e-15 and are: P, Pi, T, Ti, G, Gi, M,
Mi, k, ki, m, u, n, p, f.

The following table shows the effects of the above flags (BP, I, R) and
whether a symbol (such as "m") is specified on the parsing of some
representative inputs:

Input  | -        | +I    | +BP       | +BP,I     | "m"   | "m" +I | "m" +BP   | "m" +BP,I | "m" +R | "m" +I,R | "m" +BP,R | "m" +BP,I,R
-------|----------|-------|-----------|-----------|-------|--------|-----------|-----------|--------|----------|-----------|------------
"1M"   | 1e+6     | 1e+6  | 1,048,576 | 1,048,576 | 1e+6  | 1      | 1,048,576 | 1         | -      | 1        | -         | 1
"1k"   | 1,000    | 1,000 | 1,024     | 1,024     | 1,000 | 1,000  | 1,024     | 1,024     | -      | -        | -         | -
"1ki"  | 1,024    | 1,024 | 1,024     | 1,024     | 1,024 | 1,024  | 1,024     | 1,024     | -      | -        | -         | -
"k"    | -        | -     | -         | -         | -     | -      | -         | -         | -      | -        | -         | -
"1"    | 1        | 1     | 1         | 1         | 1     | 1      | 1         | 1         | -      | -        | -         | -
"1m"   | 0.001    | 1e+6  | -         | 1,048,576 | 1     | 1      | 1         | 1         | 1      | 1        | 1         | 1
"1u"   | 0.000001 | -     | -         | -         | -     | -      | -         | -         | -      | -        | -         | -
"1Mm"  | -        | -     | -         | -         | 1e+6  | 1e+6   | 1,048,576 | 1,048,576 | 1e+6   | 1e+6     | 1,048,576 | 1,048,576
"1km"  | -        | -     | -         | -         | 1,000 | 1,000  | 1,024     | 1,024     | 1,000  | 1,000    | 1,024     | 1,024
"1kim" | -        | -     | -         | -         | 1,024 | 1,024  | 1,024     | 1,024     | 1,024  | 1,024    | 1,024     | 1,024
"km"   | -        | -     | -         | -         | -     | -      | -         | -         | -      | -        | -         | -
"1mm"  | -        | -     | -         | -         | 0.001 | 1e+6   | -         | -         | 0.001  | 1e+6     | -         | -

An example with binary prefixes that is case insensitive:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & bytes = cli.opt<uint64_t>("b bytes")
        .siUnits("b", cli.fUnitBinaryPrefix | cli.fUnitInsensitive)
        .desc("Number of bytes to process.");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (bytes)
        cout << *bytes << " bytes\n";
    return EX_OK;
}
~~~

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -b NUM[<units>]
$ a.out -b 32768
32768 bytes
$ a.out -b 32k
32768 bytes
$ a.out -b 32KB
32768 bytes
$ a.out -b 32kib
32768 bytes
$ a.out -b 32bk
Error: Invalid '-b' value: 32bk
~~~

### Time Units
Adjusts the value to seconds when time units are present. The following units are supported:

Input | Factor
------|-------
y     | 31,536,000 (365 days, leap years not considered)
w     | 604,800 (7 days)
d     | 86,400 (24 hours)
h     | 3,600
m     | 60
min   | 60
s     | 1
ms    | 0.001
us    | 0.000001
ns    | 0.000000001

Interval in seconds where units are required:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & interval = cli.opt<uint32_t>("i interval")
        .timeUnits(cli.fUnitRequire)
        .desc("Time interval");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (interval)
        cout << *interval << " seconds\n";
    return EX_OK;
}
~~~

~~~ console
$ # Rounded to integer value so it can be stored in uint32_t
$ a.out -i 2100ms
2 seconds
$ # One year
$ a.out -i 1y
31536000 seconds
$ # You can only fit 136.2 years worth of seconds into uint32_t
$ a.out -i 137y
Error: Out of range '-i' value: 137y
Must be between '0' and '4,294,967,296'.
$ # We set fUnitRequire, so units are required...
$ a.out -i 60
Error: Invalid '-i' value: 60
~~~

### Any Units
Allows any arbitrary set of unit+factor pairs, used by both opt.siUnits() and
opt.timeUnits().

Accept length in Imperial Units:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & length = cli.opt<double>("l length")
        .anyUnits({{"yd", 36}, {"ft", 12}, {"in", 1}, {"mil", 0.001})
        .desc("Length, in inches");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (length)
        cout << *length << " inches\n";
    return EX_OK;
}
~~~

~~~ console
$ a.out
$ a.out -l 1yd
36 inches
$ a.out -l 3ft
36 inches
$ a.out -l 36
36 inches
~~~


## Counting
In very rare circumstances, it might be useful to use repetition to increase
an integer. There is no special handling for it, but counting can be done
easily enough with a vector. This can be used for verbosity flags, for
instance:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & v = cli.optVec<bool>("v verbose");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Verbosity: " << v.size();
    return EX_OK;
}
~~~
And on the command line:

~~~ console
$ a.out -vvv
Verbosity: 3
~~~

This could also be done with a [parse action](#parse-actions), but that seems
like more work.


## Prompting
You can have an option prompt the user for the value when it's left off of
the command line.

In addition to simple prompting, there are some flags that modify the behavior.

| Flag             | Description                        |
|------------------|------------------------------------|
| fPromptHide      | Hide the input from the console    |
| fPromptConfirm   | Require the value be entered twice |
| fPromptNoDefault | Don't show the default             |

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & cookies = cli.opt<int>("cookies c").prompt();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "There are " << *cookies << " cookies.";
    return EX_OK;
}
~~~
By default the prompt is a capitalized version of the first option name.
Which is why this example uses "cookies c" instead of "c cookies".

~~~ console
$ a.out -c5
There are 5 cookies.
$ a.out
Cookies [0]: 3
There are 3 cookies.
~~~
The first option name is also used in errors where no name is available from
the command line, such as when the value is from a prompt. The following
fails because "nine" isn't an int.

~~~ console
$ a.out
Cookies: nine
Error: Invalid '--cookies' value: nine
~~~
You can change the prompt to something more appropriate and hide the default:

~~~ cpp
auto & cookies = cli.opt<int>("cookies c")
    .prompt("How many cookies did you buy?", cli.fPromptNoDefault);
~~~
Which gives you:

~~~ console
$ a.out
How many cookies did you buy? 9
There are 9 cookies.
~~~


## Password Prompting
The fPromptHide and fPromptConfirm options are especially handy when asking
for passwords.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & pass = cli.opt<string>("password")
        .prompt(cli.fPromptHide | cli.fPromptConfirm);
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Password was: " << *pass;
    return EX_OK;
}
~~~
Results in:

~~~ console
$ a.out
Password:
Enter again to confirm:
Password was: secret
~~~
For passwords you can use opt.passwordOpt() instead of spelling it out.

~~~ cpp
auto & pass = cli.passwordOpt(/*confirm=*/true);
~~~
Which gives you:

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  --password=STRING  Password required for access.

  --help             Show this message and exit.
~~~


## Confirm Option
There is a short cut for a "-y, --yes" option, called cli.confirmOpt(), that
only lets the program run if the option is set or the user responds with 'y'
or 'Y' when asked if they are sure. Otherwise it sets cli.exitCode() to EX_OK
and causes cli.parse() to return false.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.confirmOpt();
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "HELLO!!!";
    return EX_OK;
}
~~~
Cover your ears...

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

Options:
  -y, --yes  Suppress prompting to allow execution.

  --help     Show this message and exit.

$ a.out -y
HELLO!!!
$ a.out
Are you sure? [y/N]: n
$ a.out
Are you sure? [y/N]: y
HELLO!!!
~~~
You can change the prompt:

~~~ cpp
cli.confirmOpt("Are loud noises okay?");
~~~
Now it asks:

~~~ console
$ a.out
Are loud noises okay? [y/N]: y
HELLO!!!
~~~


# Help Text

## Page Layout
The main help page, and the help pages for subcommands, are built the same
way and made up of the same six (not counting [option groups](#option-groups))
sections.

| Section     | Changed by | Description |
|-------------|------------|-------------|
| Header      | cli.header() | Generally a one line synopsis of the purpose of the command. |
| Usage       | cli.opt() | Generated text list the defined positional arguments. |
| Description | cli.desc() | Text describing how to use the command and what it does. Sometimes used instead of the positionals list. |
| Commands    | cli.command(), cli.desc(), opt.command() | List of commands and first line of their description, included if there are any git style subcommands. |
| Positionals | cli.opt(), opt.desc() | List of positional arguments and their descriptions, omitted if none have descriptions. |
| Options     | cli.opt(), opt.desc(), opt.valueDesc(), opt.defaultDesc(), opt.show() | List of named options and descriptions, included if there are any visible options. |
| Footer      | cli.footer() | Shown at the end, often contains references to further information. |

Within text, consecutive spaces are collapsed and words are wrapped at 80
columns. Newlines should be reserved for paragraph breaks.

~~~ cpp
Dim::Cli cli;
cli.header("Heading before usage");
cli.desc("Desciption of what the command does, including any general "
    "discussion of the various aspects of its use.");
cli.opt<bool>("[positional]");
cli.opt<string>("option").valueDesc("OPT_VAL").desc("About this option.");
cli.opt<int>("p", 1).desc("Option p.");
cli.opt<int>("q", 2).desc("Option q.").defaultDesc("two, yes TWO!");
cli.opt<int>("r", 3).desc("Option r.").defaultDesc("");
cli.footer(
    "Footer at end, usually with where to find more info.\n"
    "- first reference\n"
    "- second reference\n"
);
~~~

In this example the positionals section is omitted because the positional
doesn't have a description.

~~~ console
$ a.out --help
Heading before usage
usage: a.out [OPTIONS] positional

Description of what the command does, including any general discussion of the
various aspects of its use.

Options:
  --option=OPT_VAL  About this options.
  -p NUM            Option p. (default: 1)
  -q NUM            Option q. (default: two, yes TWO!)
  -r NUM            Option r.

  --help            Show this message and exit.

Footer at end, usually with where to find more info.
- first reference
- second reference
~~~


## Option Groups
Option groups are used to collect related options together in the help text.
In addition to name, groups have a title and sort key that determine section
heading and the order groups are rendered. Groups are created on reference,
with the title and sort key equal to the name.

Additionally there are two predefined option groups:

| Name | Sort | Title     | Description                                |
|------|------|-----------|--------------------------------------------|
| ""   | ""   | "Options" | Default group options are created          |
| "~"  | "~"  | ""        | Footer group, default location for "--help" and "--version" |

In order to generate the help text the visible options are collected into
groups, the groups are sorted by sort key and the options within each group
are sorted by name.

The group title followed by the options is then output for each group that
has options. A group without a title is still separate from the previous group
by a single blank line.

To group options you either use opt.group() to set the group name or create
the option using cli.opt&lt;T>() after changing the context with cli.group().

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.versionOpt("1.0");
    // move 1b into 'First' group after creation
    cli.opt<bool>("1b").group("First").desc("boolean 1b");
    // set context to 'First' group, update its key and add 1a directly to it
    cli.group("First").sortKey("a").title(
        "First has a really long title that wraps around to more than "
        "a single line, quite a lot of text for so few options"
    );
    cli.opt<bool>("1a");
    // add 2a to 'Second'
    cli.group("Second").sortKey("b").opt<bool>("2a");
    cli.group("Third").sortKey("c").opt<bool>("3a");
    // give the footer group a title
    cli.group("~").title("Internally Generated");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return EX_OK;
}
~~~
Let's see the groupings...

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

First has a really long title that wraps around to more than a single line,
quite a lot of text for so few options:
  --1a
  --1b       boolean 1b

Second:
  --2a

Third:
  --3a

Internally Generated:
  --help     Show this message and exit.
  --version  Show version and exit.
~~~


## Command Groups
Command groups collect related commands together in the help text, in the same
way that option groups do with options.

There are two predefined command groups:

| Name | Sort | Title      | Description                               |
|------|------|------------|-------------------------------------------|
| ""   | ""   | "Commands" | Default command group                     |
| "~"  | "~"  | ""         | Footer group, default location for "help" |

To group commands you either use cli.cmdGroup() to set the group name or create
the command using cli.command() from the context of another command that is
already in the command group that you want for the new command.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;

    // move 1a into 'First' group after creation
    cli.command("1a").cmdGroup("First").cmdSortKey("1");
    // create 1b in current 'First' group.
    cli.command("1b");
    // create 2a and move it into 'Second'
    cli.command("2a").cmdGroup("Second").cmdSortKey("2");
    // create 3a and move to 'Third'
    cli.command("3a").cmdGroup("Third").cmdSortKey("3");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return EX_OK;
}
~~~
Let's see the command groupings...

~~~ console
$ a.out --help
usage: a.out [OPTIONS]

First:
  1a
  1b

Second:
  2a

Third:
  3a

Options:
  --help     Show this message and exit.
~~~


## Help Subcommand
A simple help command can be added via cli.helpCmd(). Having a help command
allows users to run the more natural "a.out help command" to get help with a
subcommand instead of the more annoying "a.out command --help".

How to add it:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.helpCmd();
    cli.exec(cerr, argc, argv);
    return cli.exitCode();
}
~~~

Programs that only have a simple help command aren't very helpful, but it
should give you an idea. If you have more commands they will show up as you'd
expect.
~~~ console
$ a.out help
usage: a.out [OPTIONS] command [args...]

Commands:
  help      Show help for individual commands and exit.

Options:
  --help    Show this message and exit.

$ a.out help help
usage: a.out help [OPTIONS] [command]

Show help for individual commands and exit. If no command is given the list of
commands and general options are shown.
  command    Command to show help information about.

Options:
  -u, --usage / --no-usage  Only show condensed usage.

  --help                    Show this message and exit.

$ a.out help -u
usage: a.out [--help] command [args..]
$ a.out help help -u
usage: a.out help [-u, --usage] [--help] [command]
~~~


## Going Your Own Way
If generated help doesn't work for you, you can override the built-in help
with your own.

~~~ cpp
auto & help = cli.opt<bool>("help"); // or maybe "help." to suppress --no-help
if (!cli.parse(cerr, argv, argc))
    return cli.exitCode();
if (*help)
    return printMyHelp();
~~~

This works because the last definition for named options overrides any
previous ones.

Within your help printer you can use the existing functions to do some of the
work:

- cli.printHelp
- cli.printUsage / cli.printUsageEx
- cli.printPositionals
- cli.printOptions
- cli.printCommands
