# Before Actions

It's unusual to want a before action. They operate on the entire argument list, after environment variable and response file expansion, but before any individual arguments are parsed. The before action should:

* Inspect and possibly modify the raw arguments. The args are guaranteed to

  start out valid, but be careful that it still starts with a program name

  in arg0 when you're done.

* Call cli.badUsage\(\) with an error message for problems.
* Return false if the program should stop, otherwise true.

There can be any number of before actions, they are executed in the order they are added.

Let's test for empty command lines and add "--help" to them. But first, our "before" program:

```cpp
int main(int argc, char * argv[]) {
    Dim::Cli cli;
    auto & val = cli.opt<string>("<value>").desc("It's required!");
    if (!cli.parse(cerr, argc, argv))
        return cli.exitCode();
    cout << "The value: " << *val;
    return EX_OK;
}
```

And it's output:

```text
$ a.out 99
The value: 99
$ a.out --help
usage: a.out [OPTIONS]
  value     It's required!

Options:
  --help    Show this message and exit.
$ a.out
Error: Missing argument: value
```

Now add the before action:

```cpp
cli.before([](Cli &, vector<string> & args) {
    if (args.size() == 1) // it's just the program name?
        args.push_back("--help");
    return true;
});
```

And missing arguments are a thing of the past...

```text
$ a.out
usage: a.out [OPTIONS]
  value     It's required!

Options:
  --help    Show this message and exit.
```

That isn't too complicated, but for this case cli.helpNoArgs\(\) is available to do the same thing.

