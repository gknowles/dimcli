## Overview

Making command line interface implementation fun for kids of all ages.

- contained completely within namespace "Dim"
- parses directly to supplied (or implicitly created) variables
- supports parsing to any type that is:
  - default constructable
  - copyable
  - assignable from std::string or has an istream extraction operator
- help page generation

How does it feel?

~~~ cpp
#include "dim/cli.h"
#include <iostream>
using namespace std;

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & count = cli.arg<int>("c n count", 1).desc("times to say hello");
    auto & name = cli.arg<string>("name", "Unknown").desc("who to greet");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name)
        cout << "Using the unknown name." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
~~~

What it looks like when run: 

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help                   Show this message and exit.
  -c, -n, --count INTEGER  times to say hello
  --name STRING            who to greet

$ a.out --count=3
Using the unknown name.
Hello Unknown!
Hello Unknown!
Hello Unknown!
~~~


## Terminology
- Argument
  - something appearing in a command line, probably typed by a user, 
    consisting of a name and/or value.
- Positional argument
  - argument identified by their position in the command line
- Named argument
  - argument identifiable by name (e.g. --name, -n)
- Variable
  - object that receives values and contains rules about what values
    can be given to it.

The command line interface (Cli) maps arguments to variables by name
and position.


## Basic Usage
After inspecting args cli.parse() returns false if it thinks the program 
should exit, in which case cli.exitCode() is either EX_OK (because of an 
early exit like --help) or EX_USAGE for bad arguments. Otherwise the command 
line was valid, arguments have been parsed to their variables, and 
cli.exitCode() is EX_OK.

~~~ cpp
#include "dim/cli.h"
#include <iostream>
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
unixes, althrough it may not be in any standard. dimcli defines them on 
Windows if they don't exist.


## Arguments
Dim::Cli is used by declaring variables to receive arguments. Either via pointer
to a predefined external variable or by implicitly creating the variable as 
part of declaring it.

Use cli.arg\<T>(names, defaultValue) to link argument names to a target 
variable. It returns a proxy object that can be used like a pointer (* and ->) 
to access the target directly.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & fruit = cli.arg<string>("fruit", "apple");
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "Does the " << *fruit << " have a worm? No!";
    return EX_OK;
}
~~~

And what you get:

~~~ console
$ a.out --help  
Usage: a.out [OPTIONS]  

Options:  
  --help          Show this message and exit.  
  --fruit STRING  

$ a.out --fruit=orange
Does the orange have a worm? No!
~~~

Add a description and change the value's name in the description:

~~~ cpp
auto & fruit = cli.arg<string>("fruit", "apple")
    .desc("type of fruit")
    .descValue("FRUIT");
~~~
And you get:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help         Show this message and exit.  
  --fruit FRUIT  type of fruit
~~~

## External Variables
In addition to using the variable proxies you can bind the arguments directly 
to predefined variables. This can be used to set a global flag, or populate an
options struct that you access later.

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    bool worm;
    Dim::Cli cli;
    cli.arg(&worm, "w worm").desc("make it icky");
    auto & fruit = cli.arg<string>("fruit", "apple").desc("type of fruit");
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
  --help          Show this message and exit.  
  --fruit STRING  type of fruit
  -w, --worm      make it icky

$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out -w
Does the apple have a worm? Yes :(
~~~

You can also point multiple arguments at the same variable, as is common with
[feature switches](#feature-switches).

## Argument Names
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
| ?    | prefix | for non-boolean named arguments, makes the value [optional](#optional-values) |
| .    | suffix | for long names, suppresses the implicit "no-" version           |

By default, long names for boolean values get a second "no-" version implicitly
created for them.

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.arg<string>("a apple [apple]").desc("apples are red");
    cli.arg<bool>("!o orange apricot.").desc("oranges are orange");
    cli.arg<string>("<pear>").desc("pears are yellow");
    cli.parse(cerr, argc, argv);
    return EX_OK;
}
~~~
Ends up looking like this (note: required positionals are **always** placed 
before any optional ones):

~~~ console
$ a.out --help  
Usage: a.out [OPTIONS] <pear> [apple]  
  pear                        pears are yellow
  apple                       apples are red

Options:  
  --help                      Show this message and exit.  
  -a, --apple STRING          apples are red
  --apricot, --orange / -o, --no-orange  
                              oranges are orange
~~~

When named arguments are added they replace any previous rule with the same 
name, therefore these args declare '-n' an inverted bool:

~~~ cpp
cli.arg<bool>("n !n");
~~~
But with these it becomes '-n STRING', a string:

~~~ cpp
cli.arg<bool>("n !n");
cli.arg<string>("n");
~~~

## Positional Arguments
A few things to keep in mind about positional arguments:

- Arguments are mapped by the order they are added, except that all required 
  positionals appear before optional ones. 
- If there are multiple variadic positionals with unlimited (nargs = -1) 
  arity all but the first will be treated as if they had nargs = 1. 
- If the unlimited one is required it will prevent any optional positionals 
  from getting populated, since it eats up all the arguments before they get 
  a turn.


## Flag Arguments
Some (most?) arguments are flags with no associated value, they just set
a variable to a predefined value. This is the default when you create a 
variable of type bool. Normally flags set the variable to true, but this can
be changed in two ways:

- make it an inverted bool, which will set it to false
  - explicitly using the "!" modifier
  - define a long name and use the implicitly created "no-" prefix version
- use arg.flagValue() to set the value, see 
  [feature switches](#feature-switches).

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & shout = cli.arg<bool>("shout !whisper").desc("I can't hear you!");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    string prog = argv[0];
    if (*shout) {
        auto & f = use_facet<ctype<char>>(cout.getloc());
        f.toupper(prog.data(), prog.data() + prog.size());
        prog += "!!!!111";
    }
    cout << "I am " << prog;
}
~~~
What you see:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help    Show this message and exit. 
  --shout, --no-whisper / --no-shout, --whisper
            I can't hear you!

$ a.out
I am a.out
$ a.out --shout
I am A.OUT!!!!111
$ a.out --no-whisper
I am A.OUT!!!!111
~~~


## Variadic Arguments
Allows for an unlimited (or specific) number of arguments to be returned in a 
vector. Variadic arguments are declared using cli.argVec() which binds 
to a std::vector\<T>.

Example: 

~~~ cpp
// printing vectors as comma separated is annoying...
template<typename T> 
ostream & operator<< (ostream & os, const vector<T> & v) {
    auto b = v.begin();
    auto e = v.end();
    if (b != e) {
        os << *b++;
        for (; b != e; ++b) os << ", " << *b;
    }
    return os;
}

int main(int argc, char * argv[]) {
    Dim::Cli cli;
    vector<string> oranges;
    cli.argVec(&oranges, "o orange").desc("oranges");
    auto & apples = cli.argVec<string>("[apple]").desc("red fruit");
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "Comparing (" << *apples << ") and (" << *oranges << ").";
    return EX_OK;
}
~~~
View from the command line:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS] [apple...]
  apple                red fruit

Options:
  --help               Show this message and exit.
  -o, --orange STRING  oranges

$ a.out -o mandarin -onavel "red delicious" honeycrisp
Comparing (red delicious, honeycrisp) and (mandarin, navel).
~~~


## Special Arguments

| Value | Description                                               |
|-------|-----------------------------------------------------------|
| "-"   | Passed in as a positional argument.                       |
| "--"  | Thrown away, but makes all remaining arguments positional |


## Optional Values
You use the '?' [flag](#argument_names) on an argument name to indicate that
its value is optional. Only non-booleans can have optional values, booleans 
are evaluated just on their presence or absence and don't otherwise have 
values.

For a user to set a value on the command line when it is optional the value 
must be connected (no space) to the argument name, otherwise it is interpreted 
as not present and the arguments implicit value is used instead. If the name 
is not present at all the variable is set to the default given in the 
cli.arg\<T>() call. 

By default the implicit value is T{}, but can be changed using 
arg.implicitValue().

For example:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & v1 = cli.arg<string>("?o ?optional", "default");
    auto & v2 = cli.arg<string>("?i ?with-implicit", "default");
    v2.implicitValue("implicit");
    auto & p = cli.arg<string>("[positional]", "default");
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


## Feature Switches
Using flag arguments, feature switches are implemented by setting multiple
variables that reference the same target and marking them flag values.

To set the default, pass in a value of true to the flagValue() function of 
the option that should be the default.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    string fruit;
    cli.arg(&fruit, "o orange", "orange").desc("oranges").flagValue();
    cli.arg(&fruit, "a", "apple").desc("red fruit").flagValue(true); 
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "Does the " << fruit << " have a worm? No!";
    return EX_OK;
}
~~~
Which looks like:

~~~ console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help        Show this message and exit.
  -a            red fruit
  -o, --orange  oranges

$ a.out
Does the apple have a worm? No!
$ a.out -o
Does the orange have a worm? No!
~~~


## Counting
In very rare circumstances, it is interesting to use repetition to increase
an integer. There is no special handling for it, but counting can be done
easily enough with a vector. This can be used for verbosity flags, for 
instance:

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & v = cli.argVec<bool>("v verbose");
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


## Parse Actions
Sometimes, you want an argument to completely change the execution flow. For 
instance, provide more detailed errors about badly formatted arguments. Or if 
you want "--version" to print some crazy ascii artwork and exit the program 
(for a non-crazy version use [versionArg](#version_action)).

Parsing actions are attached to arguments and get invoked when a value becomes 
available for it. Any std::function compatible object that accepts references 
to cli, argument, and source value as parameters can be used. The function 
should:

- Parse the source string and use the result to set the value (or push back 
  the additional value for vectors).
- Call cli.badUsage() with an error message if there's a problem.
- Return false if the program should stop, otherwise true. You may want to 
  stop due to error or just to early out like "--version" and "--help".

Other things to keep in mind:

- You can use arg.from() to get the argument name that the value was attached
  to on the command line.
- For bool arguments the source value string will always be either "0" or "1".

Here's an action that multiples multiple values together:
~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & sum = cli.arg<int>("n number", 1)
        .desc("numbers to multiply")
        .action([](auto & cli, auto & arg, const string & val) {
            int tmp = *arg; // save the old value
            if (!arg.parseValue(val)) // parse the new value into arg
                return cli.badUsage("Bad '" + arg.from() + "' value: " + val);
            *arg *= tmp; // multiply old and new together
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
  --help            Show this message and exit.
  -n, --number=NUM  numbers to multiply

$ a.out
The product is: 1
$ a.out -n3 -n2
The product is: 6
$ a.out -nx
a.out: Bad '-n' value: x
~~~


## Version Action
Use cli.versionArg() to add simple --version processing.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    cli.versionArg("1.0");
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
a version 1.0
$ a.out
Hello world!
~~~


## Variable Modifiers

| Modifier | Done | Description |
|----------|------|-------------|
| choice | - | value from vector? index in vec for enum? or vector\<pair\<string, T>>? |
| prompt | - | prompt(string&, bool hide, bool confirm)
| passwordArg | - | arg("password").prompt("Password", true, true) |
| yes | - | are you sure? fail if not y |
| range | - | min/max allowed for variable |
| clamp | - | clamp variable so it is between min/max |

... none of these have been implemented.


## Life After Parsing
If you are using external varaibles you just access them directly after using 
cli.parse() to populate them.

If you use the proxy object returned from cli.arg\<T>() you can dereference it 
like a smart pointer to get at the value. In addition, you can test whether 
it was explicitly set and get the argument name that populated it.

~~~ cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & name = cli.arg<string>("n name", "Unknown");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name) {
        cout << "Using the unknown name." << endl;
    } else {
        cout << "Name selected using " << name.from() << endl;
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
Name selected using --name
Hello Mary!
~~~


## Keep it quiet
For some applications, such as Windows services, it's important not to 
interact with the console. Simple steps to avoid parse() doing console IO:

1. Don't use options (such as arg.prompt()) that explicitly ask for IO.
2. Add your own "help" argument to override the default, you can still 
turn around and call cli.writeHelp(ostream&) if desired.
3. Use the two argument version of cli.parse() and get the error message from
cli.errMsg() if it fails.
