<!--
Copyright Glen Knowles 2016 - 2017.
Distributed under the Boost Software License, Version 1.0.
-->

# Overview
C++ command line parser toolkit for kids of all ages.

- can parse directly to any supplied (or implicitly created) variables 
  that are:
  - default constructable
  - copyable
  - assignable from std::string or have an istream extraction operator
- help generation
- option definitions can be scattered across multiple files
- git style subcommands
- response files
- works with exceptions and/or RTTI enabled or disabled

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
        cout << "Using the unknown name." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return 0;
}
~~~
What that does when run: 

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  -c, -n, --count=INTEGER  times to say hello (default: 1)
  --name STRING            who to greet (default: Unknown)

  --help                   Show this message and exit.

$ a.out --count=3
Using the unknown name.
Hello Unknown!
Hello Unknown!
Hello Unknown!
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

The EX_* constants (along with standard values) are in sysexits.h on most 
unixes, althrough it may not be in any standard. Equivalent enum values
Dim::kExitOk (0) and Dim::kExitUsage (64) are defined, which can be useful on 
Windows where \<sysexits.h> doesn't exist.


## Options
Dim::Cli is used by declaring options to receive arguments. Either via 
pointer to a predefined external variable or by implicitly creating the 
variable when the option is declared. 

Use cli.opt\<T>(names, defaultValue) to link positional or named arguments to 
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
[feature switches](Feature%20Switches).


## Option Names
Names are passed in as a space separated list where the individual names look 
like these:

| Type of name                        | Example      |
|-------------------------------------|--------------|
| short name (single character)       | f            |
| long name (more than one character) | file         |
| optional positional                 | \[file name] |
| required positional                 | \<file\>     |

Names for positionals (inside angled or square brackets) may contain spaces, 
and all names may have modifier flags:

| Flag | Type   | Description                                                     |
| :--: |--------|-----------------------------------------------------------------|
| !    | prefix | for boolean values, when setting the value it is first inverted |
| ?    | prefix | for non-boolean named options, makes the value [optional](Optional%20Values) |
| .    | suffix | for long names, suppresses the implicit "no-" version           |

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
  all but the first will be treated as if they had nargs = 1. 
- If the unlimited one is required it will prevent any optional positionals 
  from getting populated, since it eats up all the arguments before they get 
  a turn.


## Flag Arguments
Many arguments are flags with no associated value, they just set an option 
to a predefined value. This is the default when you create a option of type 
bool. Normally flags set the option to true, but this can be changed in two 
ways:

- make it an inverted bool, which will set it to false
  - explicitly using the "!" modifier
  - define a long name and use the implicitly created "no-" prefix version
- use opt.flagValue() to set the value, see 
  [feature switches](Feature%20Switches).

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
std::vector\<T>.

Example: 

~~~ cpp
// printing a comma separated list is annoying...
template<typename T> 
ostream & operator<< (ostream & os, const vector<T> & v) {
    auto i = v.begin(), e = v.end();
    if (i != e) {
        os << *i++;
        for (; i != e; ++i) os << ", " << *i;
    }
    return os;
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    // for oranges demonstrate using a separate vector
    vector<string> oranges;
    cli.optVec(&oranges, "o orange").desc("oranges");
    // for apples demonstrate just using the proxy object
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
  -o, --orange=STRING  oranges

  --help               Show this message and exit.

$ a.out -o mandarin -onavel "red delicious" honeycrisp
Comparing (red delicious, honeycrisp) and (mandarin, navel).
~~~

While the * and -> operators get you full access to the underlying vector, 
size() and [] are also available directly on the OptVec<T>. Which may 
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

If you use the proxy object returned from cli.opt\<T>() you can dereference it 
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


# Advanced

## Special Arguments

| Value      | Description                                                      |
|------------|------------------------------------------------------------------|
| "-"        | Passed in as a positional argument.                              |
| "--"       | Thrown away, but makes all remaining arguments positional        |
| "@\<file>" | [Response file](Response%20Files) containing additional arguments  |


## Optional Values
You use the '?' [flag](Option%20Names) on an argument name to indicate that
its value is optional. Only non-booleans can have optional values, booleans 
are evaluated just on their presence or absence and don't otherwise have 
values.

For a user to set a value on the command line when it is optional the value 
must be connected (no space) to the argument name, otherwise it is interpreted 
as not present and the arguments implicit value is used instead. If the name 
is not present at all the variable is set to the default given in the 
cli.opt\<T>() call. 

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


## Parse Actions
Sometimes, you want an argument to completely change the execution flow. For 
instance, to provide more detailed errors about badly formatted arguments. Or 
to make "--version" print some crazy ascii artwork and exit the program (for 
a non-crazy --version use [opt.versionOpt()](Version%20Option)).

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
        .parse([](auto & cli, auto & opt, const string & val) {
            int tmp = *opt; // save the old value
            if (!opt.parseValue(val)) // parse the new value into opt
                return cli.badUsage("Bad '" + opt.from() + "' number: " + val);
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
Error: Bad '-n' number: x
~~~


## Check Actions
Check actions run for each value that is successfully parsed and are a good 
place for additional work. For example, opt.range() and opt.clamp() are 
implemented as check actions. Just like parse actions the callback is any
std::function compatible object that accepts references to cli, opt, and
string parameters and returns bool. 

An option can have any number of check actions and they are called in the
order they were added.

The function should:
- check the options new value, possible in relation to other options.
- call cli.badUsage() with an error message if there's a problem.
- Return false if the program should stop, otherwise true to let processing
  continue.

The opt is fully populated, so *opt, opt.from(), etc are all available.


## After Actions
After actions run after all arguments have been parsed. For example, 
opt.prompt() and opt.require() are both implemented as after actions. Any 
number of after actions can be added and will, for each option, be called in 
the order they're added. They are called with the three parameters, like other 
option actions, that are references to cli, opt, and source value respectively. 
However the const string& source is always empty(), so any information about 
the value must come from the opt reference.

When using subcommands, only the after actions bound to the top level or the
selected command are executed. After actions on the options of all other 
commands are, like the options themselves, ignored.

The function should:
- Do something interesting.
- Call cli.badUsage() and return false on error.
- Return true if processing should continue.


## Multiple Source Files
Options don't have to be defined all in one source file, separate source 
files can each define options of interest to that file and get them populated
when the command line is processed.

When you instanticate Dim::Cli you're creating a handle to the globally 
shared configuration. So multiple translation units can each create one and
use it to update the shared configuration.

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

#### Dim::CliLocal
Although it was created for testing you can also use Dim::CliLocal, a 
completely self-contained parser, if you need to redefine options, have 
results from multiple parses at once, or otherwise need to avoid the shared 
configuration.


## Subcommands
Git style subcommands are created by either cli.command("cmd"), which changes
the cli objects context to the command, or with opt.command("cmd"), which 
changes the command the option is for. Once the cli object context has been 
changed it can than be used to add (desciption, footer, options, etc) to the 
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
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    return cli.run();
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

The end result from the console:

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

In the commands list, the cli.desc() is only used up to the first period, but 
in command specific pages you see the whole thing:

~~~ console
$ a.out apple --help
usage: a.out apple [OPTIONS]
Show apple. No other fruit.

Options:
  --color=STRING  Change color of the apple. (default: red)

  --help          Show this message and exit.
~~~


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
prepended to the command line. 

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
if (const char * eopts = getenv("AOUT_OPTS")) {
    vector<string> eargs = cli.toArgv(eopts);
    // Insert the environment args after arg0 (program name) but before
    // the rest of the command line.
    args.insert(args.begin() + 1, eargs.begin(), eargs.end());
}
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
desciption or option group, but since you get back an Opt\<T> you can use any 
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


## Feature Switches
Using flag arguments, feature switches are implemented by creating multiple
options that reference the same external variable and marking them flag 
values.

To set the default, pass in a value of true to the flagValue() function of 
the option that should be the default.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    string fruit;
    // "~" is default option group for --help, --version, etc. It defaults to 
    // an empty title, give it one so it doesn't look like more fruit.
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


## Choice Options
Sometimes you want an option to have a fixed set of possible values, such as
for an enum. You use opt.choice() to add legal choices, one at a time, to an
option.

Choices are similar to [feature switches](Feature%20Switches) but instead of
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
Error: Out of range 'letter' value [a - z]: 1
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

This could also be done with a [parse action](Parse%20Actions), but that seems
like more work.


## Prompting
You can have an option prompt the user for the value when it's left off of 
the command line.

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
Cookies: 3
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
You can change the prompt to something more appropriate:

~~~ cpp
auto & cookies = cli.opt<int>("cookies c")
    .prompt("How many cookies did you buy?");
~~~
Which gives you:

~~~ console
$ a.out
How many cookies did you buy? 9
There are 9 cookies.
~~~


## Password Prompting
In addition to simple prompting, when an option is left off the command line 
a prompt also has some behaviors that are controlled by flags.

| Flag           | Description                        |
|----------------|------------------------------------|
| kPromptHide    | hides the input fron the console   |
| kPromptConfirm | require the value be entered twice |

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & pass = cli.opt<string>("password")
        .prompt(cli.kPromptHide | cli.kPromptConfirm);
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
usage: test [OPTIONS]

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
way and made up of the same six (not counting [option groups](Option%20Groups))
sections.

| Section | Changed by | Description |
|---------|------------|-------------|
| Header | cli.header() | Generally a one line synopsis of the purpose of the command. |
| Usage | cli.opt() | Generated text list the defined positional arguments. |
| Description | cli.desc() | Text descirbing how to use the command and what it does. Sometimes used instead of the positionals list. |
| Positionals | cli.opt(), opt.desc() | List of positional arguments and their descriptions, omitted if none have descriptions. |
| Options | cli.opt(), opt.desc(), opt.valueDesc(), opt.show() | List of named options and descriptions, included if there are any visible options. |
| Commands | cli.command(), cli.desc(), opt.command() | List of commands and first line of their description, included if there are any git style subcommands. |
| Footer | cli.footer() | Shown at the end, often contains references to further information. |

Within text, consecutive spaces are collapsed and words are wrapped at 80 
columns. Newlines should be reserved for paragraph breaks.

~~~ cpp
Dim::Cli cli;
cli.header("Heading before usage");
cli.desc("Desciption of what the command does, including any general "
    "discussion of the various aspects of its use.");
cli.opt<bool>("[positional]");
cli.opt<string>("option").valueDesc("OPT_VAL").desc("About this option.");
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

| Name | Sort | Title     | Desciption                                 |
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
the option using cli.opt\<T>() after changing the context with cli.group().

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


## Going Your Own Way
If generated help doesn't work for you, you can override the builtin help 
option with your own.

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


## Help Subcommand
There is no default help subcommand, but you can make a basic one without much
trouble.

One way to do it:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.command("help");
    cli.desc("This is how you get help. There could be more details.")
    auto & cmd = cli.opt<string>("[command]").desc("Command to explain.");
    cli.action([&cmd](auto & cli) {
        return cli.printHelp(cout, {}, *cmd);
    });
    if (!cli.parse(argc, argv))
        return cli.exitCode();
    return cli.run();
}
~~~

And what you get:

~~~ console
$ a.out --help
usage: a.out [OPTIONS] command [args...]

Options:
  --help    Show this message and exit.

Commands:
  help      This is how you get help.

$ a.out help help
usage: a.out help [OPTIONS] command
This is how you get help. There could be more details.
  command   Command to explain.

Options:
  --help    Show this message and exit.
~~~
