# dim-cli

Module for making unix style command line interfaces.

Main features:
- parses directly to c++ variables (or makes proxies for them)
- supports parsing to any type that is:
  - default constructable
  - assignable from std::string or has an istream extraction operator
- help page generation

What does it look like?
```C++
int main(int argc, char * argv[]) {
    Cli cli;
    auto & count = cli.arg<int>("c count", 1).desc("times to say hello");
    auto & name = cli.arg<string>("name", "Unknown").desc("who to greet");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name)
        cout << "Using the unknown name." << endl;
    for (unsigned i = 0; i < *count; ++i)
        cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
```

What it looks like when run: 
```console
$ a.out --count=3
Using the unknown name.
Hello Unknown!
Hello Unknown!
Hello Unknown!
```

It automatically generates reasonably formatted help pages:
```console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help               Show this message and exit.
  -c, --count INTEGER  times to say hello
  --name STRING        who to greet
```

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
After inspecting args Cli::parser() returns false if it thinks the program 
should exit. Cli::exitCode() will be set to either EX_OK (because of an early 
exit like --help) or EX_USAGE for bad arguments.

```C++
int main(int argc, char * argv[]) {
    Cli cli;
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the apple have a worm? No!";
    return EX_OK;
}
```

And what it looks like:
```console
$ a.out --help  
Usage: a.out [OPTIONS]  

Options:  
  --help    Show this message and exit.  
$ a.out
Does the apple have a worm? No!
```

## Arguments
Cli is used by declaring variables to receive arguments. Either via pointer
to a predefined external variable or by implicitly creating the variable as 
part of declaring it.

Use arg<T>(names, defaultValue) to link argument names to a target variable. 
It returns a proxy object that can be used like a pointer (* and ->) to 
access the target directly.

```C++
int main(int argc, char * argv[]) {
    Cli cli;
    auto & fruit = cli.arg<string>("fruit", "apple");
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "Does the " << *fruit << " have a worm? No!";
    return EX_OK;
}
```

And what you get:
```console
$ a.out --help  
Usage: a.out [OPTIONS]  

Options:  
  --help          Show this message and exit.  
  --fruit STRING  
$ a.out --fruit=orange
Does the orange have a worm? No!
```

Add a description:
```C++
auto & fruit = cli.arg<string>("fruit", apple").desc("type of fruit");
```
And you get:
```console
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --help          Show this message and exit.  
  --fruit STRING  type of fruit
```

## External Variables
In addition to using the variable proxies you can bind the arguments directly 
to predefined variables. This can be used to set a global flag, or populate an
options struct that you access later.

You can also point multiple arguments at the same variable, as is common with
[feature switches](#feature-switches).

For example:
```C++
int main(int argc, char * argv[]) {
    bool worm;
    Cli cli;
    cli.arg(&worm, "w worm").desc("make it icky");
    auto & fruit = cli.arg<string>("fruit", "apple").desc("type of fruit");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << *fruit << " have a worm? " 
        << worm ? "Yes :(" : "No!";
    return EX_OK;
}
```
And what it looks like:
```console
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
```

## Argument Names
Names are passed in as a space separated list of argument names that look 
like these:

| Type of name                        | Example     |
|-------------------------------------|-------------|
| short name (single character)       | f           |
| long name (more than one character) | file        |
| optional positional                 | [file name] |
| required positional                 | \<file\>    |

Names for positionals (inside angled or square brackets) may contain spaces, 
and all names may be preceded by modifier flags:

| Flag | Description                                                     |
|------|-----------------------------------------------------------------|
| !    | for boolean values, when setting the value it is first inverted |

Long names for boolean values get a second "no-" version implicitly
created for them.

For example:
```C++
int main(int argc, char * argv[]) {
    Cli cli;
    cli.arg<string>("a apple [apple]").desc("apples are red");
    cli.arg<bool>("!o orange").desc("oranges are orange");
    cli.arg<string>("<pear>").desc("pears are yellow");
    cli.parse(cerr, argc, argv);
    return EX_OK;
}
```
Ends up looking like this (note: required positionals are **always** placed 
before any optional ones):
```console
$ a.out --help  
Usage: a.out [OPTIONS] <pear> [apple]  
  pear      pears are yellow
  apple     apples are red

Options:  
  --help              Show this message and exit.  
  -a, --apple STRING  apples are red
  -o, --[no-]orange   oranges are orange
```

When named arguments are added they replace any previous rule with the same 
name, therefore these args declare '-n' an inverted bool:
```C++
cli.arg<bool>("n !n");
```
But with these it becomes '-n STRING', a string:
```C++
cli.arg<bool>("n !n");
cli.arg<string>("n");
```

## Positional Arguments
Arguments are mapped by the order they are added, with the exception that
all required positionals appear before optional ones. If there are multiple
variadic positionals with unlimited (nargs = -1) arity all but the first will 
be treated as if they had nargs = 1. Also, if the unlimited one is required
it will prevent any optional positionals from getting populated, since it eats 
up all the arguments before they get a turn.


## Flag Arguments
Some (most?) arguments are flags with no associated value, they just set
a variable to a predefined value. This is the default when you create a 
variable of type bool. Normally flags set the variable to true, but this can
be changed in two ways:  
- make it an inverted bool, which will set it to false
  - explicitly using the "!" modifier
  - define a long name and use the implicitly created "no-" prefix version
- use Arg<T>::flagValue() to set the value, see 
  [feature switches](#feature-switches).

```C++
int main(int argc, char * argv[]) {
    Cli cli;
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
```
What you see:
```console
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
```


## Variadic Arguments
Allows for a specific (or unlimited) number of arguments to be returned in a 
vector. Variadic arguments are declared using Cli::argVec() which binds 
to a std::vector<T>.

Example: 
```C++
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
    Cli cli;
    vector<string> oranges;
    cli.argVec(&oranges, "o orange").desc("oranges");
    auto & apples = cli.argVec<string>("[apple]").desc("red fruit");
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "(" << *apples << ") and (" << *oranges << ") don't compare.";
    return EX_OK;
}
```
And from the command line:
```console
$ a.out --help
Usage: a.out [OPTIONS] [apple...]
  apple     red fruit

Options:
  --help               Show this message and exit.
  -o, --orange STRING  oranges
$ a.out -o mandarin -onavel "red delicious" honeycrisp
(red delicious, honeycrisp) and (mandarin, navel) don't compare.
```


## Special Arguments
| Value | Description                                               |
|-------|-----------------------------------------------------------|
| "-"   | Passed in as a positional argument.                       |
| "--"  | Thrown away, but makes all remaining arguments positional |


## Optional Values
When an optional argument is not passed in the variable is still set to the 
default in the Cli::arg() call. In order to set an optional value it must be 
connected (no space) to the name, otherwise it is interpreted as not present 
and the implicit value is used instead. The implicit value can be set in the 
same Arg<T>::optional() call you use to make it optional.

For example:
```C++
int main(int argc, char * argv[]) {
    Cli cli;
    auto & v1 = cli.arg<string>("o optional", "default").optional();
    auto & v2 = cli.arg<string>("i with-implicit", "default");
    v2.optional("implicit");
    auto & p = cli.arg<string>("[positional]", "default");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "v1 = " << *v1 << ", v2 = " << *v2 << ", p = " << *p;
    return EX_OK;
}
```
What happens: 
```console
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
```


## Feature Switches
Using flag arguments, feature switches are implemented by setting multiple
variables that reference the same target and marking them flag values.

To set the default, pass in a value of true to the flagValue() function of 
the option that should be the default.

```C++
int main(int argc, char * argv[]) {
    Cli cli;
    string fruit;
    cli.arg(&fruit, "o orange", "orange").desc("oranges").flagValue();
    cli.arg(&fruit, "a", "apple").desc("red fruit").flagValue(true); 
    if (!cli.parse(cerr, argc, argv)) {
        return cli.exitCode();
    }
    cout << "Does the " << fruit << " have a worm? No!";
    return EX_OK;
}
```
Which looks like:
```console
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
```


## Counting
In very rare circumstances, it is interesting to use repetition to increase
an integer. There is no special handling for it, but counting can be done
easily enough with a vector. This can be used for verbosity flags, for instance:
```C++
int main(int argc, char * argv[]) {
    Cli cli;
    auto & v = cli.argVec<bool>("v verbose");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Verbosity: " << v.size();
    return EX_OK;
}
```
And on the command line:
```console
$ a.out -vvv
Verbosity: 3
```


## Variable Modifiers

| Modifier | Done | Description |
|----------|------|-------------|
| choice | - | value from vector? index in vec for enum? or vector<pair<string, T>>? |
| prompt | - | prompt(string&, bool hide, bool confirm)
| argPassword | - | arg<string>("password").prompt("Password", true, true) |
| yes | - | are you sure? fail if not y |
| range | - | min/max allowed for variable |
| clamp | - | clamp variable so it is between min/max |


## Life After Parsing
If you are using external varaibles you can just access them directly after 
using Cli::parse() to populate them.

If you use the argument object returned from Cli::arg() you dereference it 
like a smart pointer to get at the value. In addition, you can test whether 
it was explicitly set and get the name that was used.

```C++
int main(int argc, char * argv[]) {
    Cli cli;
    auto & name = cli.arg<string>("n name", "Unknown");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    if (!name) {
        cout << "Using the unknown name." << endl;
    } else {
        cout << "Name selected using " << name << endl;
    }
    cout << "Hello " << *name << "!" << endl;
    return EX_OK;
}
```
What it does: 
```console
$ a.out  
Using the unknown name.  
Hello Unknown!  
$ a.out -n John
Name selected using -n
Hello John!
$ a.out --name Mary
Name selected using --name
Hello Mary!
```

## How to keep Cli::parse() from doing IO
For some applications, such as Windows services, it's important not to 
interact with the console. There some simple steps to stop parse() from doing 
any console IO:

1. Don't use options (such as Arg<T>::prompt()) that explicitly ask for IO.
2. Add your own "help" argument to override the default, you can still 
turn around and call Cli::writeHelp(ostream&) if desired.
3. Use the two argument version of Cli::parse() so it doesn't output errors 
immediately. The text of any error that may have occurred during a parse is 
available via Cli::errMsg()


## Aspirational Roadmap
- callbacks
- help text and formatting options
- load from environment variables?
- support /name:value syntax
- composable subcommands
- access to unrecognized named options?
- tuple arguments
