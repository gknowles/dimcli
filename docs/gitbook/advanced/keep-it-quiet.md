# Keep It Quiet

For some applications, such as Windows services, it's important not to interact with the console. Simple steps to avoid cli.parse\(\) doing console IO:

1. Don't use things \(such as opt.prompt\(\)\) that explicitly ask for IO.
2. Add your own "help" argument to override the default, you can still turn

   around and call cli.printHelp\(ostream&\) if desired.

3. Use the two argument version of cli.parse\(\) and get the error message from

   cli.errMsg\(\) and cli.errDetail\(\) if it fails.

