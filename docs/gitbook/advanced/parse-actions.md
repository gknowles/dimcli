# Parse Actions

Sometimes, you want an argument to completely change the execution flow. For instance, to provide more detailed errors about badly formatted arguments. Or to make "--version" print some crazy ASCII artwork and exit the program \(for a non-crazy --version use [opt.versionOpt\(\)](https://github.com/gknowles/dimcli/tree/b6fa17b725368b913b0367993b82158b0cb14455/docs/gitbook/Advanced/Version%20Option/README.md)\).

Parsing actions are attached to options and get invoked when a value becomes available for it. Any std::function compatible object that accepts references to cli, opt, and string as parameters can be used. The function should:

* Parse the source string and use the result to set the option value \(or

  push back the additional value for vector arguments\).

* Call cli.badUsage\(\) with an error message if there's a problem.
* Return false if the program should stop, otherwise true. You may want to

  stop due to error or just to early out like "--version" and "--help".

Other things to keep in mind:

* Options only have one parse action, changing it _replaces_ the default.
* You can use opt.from\(\) and opt.pos\(\) to get the argument name that the value

  was attached to on the command line and its position in argv\[\].

* For bool options the source value string will always be either "0" or "1".

Here's an action that multiples multiple values together:

```cpp
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
```

Let's do some math!

```text
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
```

