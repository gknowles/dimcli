# Options

Dim::Cli is used by declaring options to receive arguments. Either via pointer to a predefined external variable or by implicitly creating the variable when the option is declared.

Use cli.opt\\(names, defaultValue\) to link positional or named arguments to an option. It returns a proxy object that can be used like a pointer \(\* and -&gt;\) to access the value.

```cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & fruit = cli.opt<string>("fruit", "apple");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "Does the " << *fruit << " have a worm? No!";
    return EX_OK;
}
```

And what you get:

```text
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --fruit=STRING  (default: apple)

  --help          Show this message and exit.

$ a.out --fruit=orange
Does the orange have a worm? No!
$ a.out --fruit orange
Does the orange have a worm? No!
```

Add a description and change the value's name in the description:

```cpp
auto & fruit = cli.opt<string>("fruit", "apple")
    .desc("type of fruit")
    .valueDesc("FRUIT");
```

And you get:

```text
$ a.out --help
Usage: a.out [OPTIONS]

Options:
  --fruit=FRUIT  type of fruit (default: apple)

  --help         Show this message and exit.
```

