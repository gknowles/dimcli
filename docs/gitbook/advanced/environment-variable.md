# Environment Variable

You can specify an environment variable that will have its contents prepended to the command line. This happens before response file expansion and any before actions.

```cpp
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
```

The same can also be done manually, as shown below. This is a good starting point if you need something slightly different:

```cpp
vector<string> args = cli.toArgv(argc, argv);
if (char const * eopts = getenv("AOUT_OPTS")) {
    vector<string> eargs = cli.toArgv(eopts);
    // Insert the environment args after arg0 (program name) but before
    // the rest of the command line.
    args.insert(args.begin() + 1, eargs.begin(), eargs.end());
}
if (!cli.parse(cerr, args))
    return cli.exitCode();
```

Or as a before action \(after response file expansion\):

```cpp
cli.before([](Cli &, vector<string> & args) {
    if (char const * eopts = getenv("AOUT_OPTS")) {
        vector<string> eargs = cli.toArgv(eopts);
        args.insert(args.begin() + 1, eargs.begin(), eargs.end());
    }
});
if (!cli.parse(cerr, args))
    return cli.exitCode();
```

How this works:

```text
$ export AOUT_OPTS=
$ a.out c d
Words: 'c' 'd'
$ export AOUT_OPTS=a b
$ a.out 'c' 'd'
Words: 'a' 'b' 'c' 'd'
```

