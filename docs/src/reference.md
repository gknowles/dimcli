<!--
Copyright Glen Knowles 2019.
Distributed under the Boost Software License, Version 1.0.
-->

# Reference

## Classes

Class | Description
------|------------
[Cli](guide.md#basic-usage) | Creates a handle to the shared command line configuration, this indirection allows options to be statically registered from multiple source files.
[CliLocal](guide.md#dimclilocal) | Stand-alone cli instance independent of the shared configuration. Mainly for testing.
Cli::Opt&lt;T> | Metadata and reference to single value option.
Cli::OptVec&lt;T> | Metadata and reference to vector of values for multivalued options.

## Subcommands

Create | &nbsp;
---------|------------
cli.[command](guide.md#subcommands) | Changes config context to reference the options of the selected command. Use "" to specify the top level context. If a new command is selected it will be created in the command group of the current context.
cli.[helpCmd](guide.md#help-subcommand) | Add "help" command that shows the help text for other commands. Allows users to run "prog help command" instead of the more awkward "prog command --help".
<b>Configuration</b> |
cli.[action](guide.md#subcommands) | Action that should be taken when the currently selected command is run. Actions are executed when cli.exec() is called by the application.

## Options

Create | &nbsp;
---------|------------
cli.[opt&lt;T>](guide.md#options) | Add command line option
cli.[optVec&lt;T>](guide.md#vector-options) | Add multivalued option
cli.[confirmOpt](guide.md#confirm-option) | Add -y, --yes option that exits early when false and has an "are you sure?" style prompt when it's not present.
cli.[helpOpt](guide.md#help-option) | Get reference to internal help option, can be used to change the description, option group, etc. To completely replace it, add another option that responds to --help.
cli.[passwordOpt](guide.md#password-prompting) | Add --password option and prompts for a password if it's not given on the command line. If confirm is true and it's not on the command line you have to enter it twice.
cli.[versionOpt](guide.md#version-option) | Add --version option that shows "${progName.filename()} version ${ver}" and exits. An empty progName defaults to argv[0].
<b>Configuration</b> |
opt.[after](guide.md#after-actions) | Action to run after all arguments have been parsed, any number of after actions can be added and will, for each option, be called in the order they're added.
opt.[check](guide.md#check-actions) | Action to take immediately after each value is parsed, unlike parsing itself where there can only be one action, any number of check actions can be added.
opt.[choice](guide.md#choice-options) | Adds a choice, when choices have been added only values that match one of the choices are allowed. Useful for things like enums where there is a controlled set of possible values.
opt.[clamp](guide.md#range-and-clamp) | Forces the value to be within the range, if it's less than the low it's set to the low, if higher than high it's made merely high.
opt.defaultValue | Allows the default to be changed after the opt has been created.
opt.[flagValue](guide.md#feature-switches) | Turns the argument into a feature switch, there are normally multiple switches pointed at a single external value, one of which should be flagged as the default.
opt.[implicitValue](guide.md#optional-values) | The implicit value is used for arguments with optional values when the argument was specified in the command line without a value.
opt.[parse](guide.md#parse-actions) | Change the action to take when parsing this argument.
opt.[prompt](guide.md#prompting) | Enables prompting. When the option hasn't been provided on the command line the user will be prompted for it. Use Cli::fPrompt* flags to adjust behavior.
opt.[range](guide.md#range-and-clamp) | Fail if the value given for this option is not in within the range (inclusive) of low to high.
opt.[require](guide.md#require) | Causes a check whether the option value was set during parsing, and reports badUsage() if it wasn't.

## Parsing and execution

Application | &nbsp;
------------|-------
cli.[exec](guide.md#subcommands) | Optionally parses, and then executes the action of the selected command; returns true if it worked. On failure it's expected to have set exitCode, errMsg, and optionally errDetail via fail().
cli.[parse](guide.md#basic-usage) | Parse the command line, populate the options, and set the error and other miscellaneous state. Returns true if processing should continue.
cli.resetValues | Sets all options to their defaults, called internally when parsing starts.
<b>Actions</b> |
cli.[badUsage](guide.md#after-actions) | Intended for use from return statements in action callbacks. Sets exit code (to EX_USAGE) and error msg, then returns false.
cli.commandExists | Returns true if the named command has been defined, used by the help command implementation. Not reliable before cli.parse() has been called and had a chance to update the internal data structures.
cli.parseValue | Used to populate an option with an arbitrary input string through the standard parsing logic. Since it causes the parse and check actions to be called care must be taken to avoid infinite recursion if used from those actions. Can be used from after actions to simulate a value.
cli.prompt | Prompt sends a prompt message to cout and read a response from cin (unless cli.iostreams() changed the streams to use), the response is then passed to cli.parseValue() to set the value and run any actions.
opt.defaultFrom | Get default from to use when populating the option from an action that's not tied to a command line argument.
opt.defaultPrompt | Get default name to use in prompts when not tied to a command line.
opt.[parseValue](guide.md#parse-actions) | Parse the string into the value, return false on error.
opt.reset | Set option to its default value.
opt.unspecifiedValue | Set option (or add to option vector) to value for missing optionals.
<b>Commands</b> |
cli.fail | Sets exitCode(), errMsg(), and errDetail(), intended to be called from command actions, parsing related failures should use badUsage().
<b>After parsing</b> |
cli.[exitCode](guide.md#basic-usage) | EX_OK (0), EX_USAGE, or any value set by user defined actions.
cli.errMsg | Error message, only meaningful when exitCode() != EX_OK
cli.errDetail | Additional information that may help the user correct their mistake, may be empty.
cli.progName | Program name received in argv[0]
cli.commandMatched | Command to run, as selected by argv, empty string if there are no commands defined or none were selected.
opt.[operator&nbsp;bool](guide.md#life-after-parsing) | True if the value was populated from the command line, whether the resulting value is the same as the default is immaterial.
opt.[operator *](guide.md#life-after-parsing) | Reference to underlying value or, for OptVec, vector of values.
opt.[operator ->](guide.md#life-after-parsing) | Pointer to underlying value or value vector.
opt.[operator []](guide.md#vector-options) | Array access to members of value vector (OptVec only).
opt.[from](guide.md#life-after-parsing) | Name of the last argument to populated the value, or an empty string if it wasn't populated. For vectors, it's what populated the last value.
opt.[pos](guide.md#life-after-parsing) | Absolute position in argv[] of last the argument that populated the value. For vectors, it refers to where the value on the back came from. If pos() is 0 the value wasn't populated from the command line or wasn't populated at all, check from() to tell the difference.
opt.[size](guide.md#life-after-parsing) | Number of values, non-vectors are always 1.

## Help Text

Command&nbsp;groups | &nbsp;
---------|-------
cli.[cmdGroup](guide.md#command-groups) | Changes the command group of the current command. Because new commands start out in the same group as the current command, it can be convenient to create all the commands of one group before moving to the next.
cli.[cmdSortKey](guide.md#command-groups) | Command groups are sorted by key, defaults to group name.
cli.[cmdTitle](guide.md#command-groups) | Heading title to display, defaults to group name. If empty there will be a single blank line separating this group from the previous one.
<b>Commands</b> |
cli.[header](guide.md#page-layout) | Arbitrary help text, for the command, before the usage section.
cli.[desc](guide.md#page-layout) | Help text, for the command, between the usage and arguments / options.
cli.[footer](guide.md#page-layout) | Help text, for the command, after the options.
<b>Option groups</b> |
cli.[group](guide.md#option-groups) | Changes configuration context to point at the selected option group of the current command.
cli.[sortKey](guide.md#option-groups) | Sets sort key of current option group. Option groups are sorted by key, defaults to group name.
cli.[title](guide.md#option-groups) | Sets heading title for current option group to display, defaults to group name. If empty there will be a single blank line separating this group from the previous one.
<b>Options</b> |
opt.show | Controls whether or not the option appears in help pages.
opt.[command](guide.md#subcommands) | Set subcommand for which this is an option.
opt.[group](guide.md#option-groups) | Set group under which this argument will show up in the help text.
opt.[desc](guide.md#page-layout) | Set description to associate with the argument in help text.
opt.[valueDesc](guide.md#page-layout) | Set name of meta-variable in help text. For example, in "--count NUM" this is used to change "NUM" to something else.
opt.[defaultDesc](guide.md#page-layout) | Set text to appear in the default clause of this options the help text. Can change the "0" in "(default: 0)" to something else, or use an empty string to suppress the entire clause.
<b>Rendering</b>
cli.[printHelp](guide.md#going-your-own-way) | Write help page for selected command to std::ostream&amp;
cli.[printUsage](guide.md#going-your-own-way) | Write simple usage.
cli.[printUsageEx](guide.md#going-your-own-way) | Write usage, but include names of all non-default options.
cli.[printPositionals](guide.md#going-your-own-way) | Write names and descriptions of positional arguments.
cli.[printOptions](guide.md#going-your-own-way) | Write full option descriptions.
cli.[printCommands](guide.md#going-your-own-way) | Write names and descriptions of commands
cli.[printError](guide.md#going-your-own-way) | If exitCode() is not EX_OK, prints the errMsg and errDetail (if present), otherwise does nothing. Returns exitCode(). Only makes sense after parsing has completed.

## Configuration

Miscellaneous | &nbsp;
---------|------------
cli.[before](guide.md#before-actions) | Actions taken after environment variable and response file expansion but before any individual arguments are parsed.
cli.conin | Get console input stream that will be used for prompting.
cli.conout | Get console output stream that will be used for prompting.
cli.[envOpts](guide.md#environment-variable) | Environment variable to get initial options from. Defaults to the empty string, but when set the content of the named variable is parsed into args which are then inserted into the argument list right after arg0.
cli.[helpNoArgs](guide.md#help-option) | Adds before action that replaces the empty command line with "--help".
cli.imbue | Change the locale used when parsing values via iostream. Defaults to the user's preferred locale (aka locale("")).
cli.iostreams | Changes the streams used for prompting, printing help messages, etc. Mainly intended for testing. Setting to null restores the defaults which are cin and cout respectively.
cli.[responseFiles](guide.md#response-files) | Enabled by default, response file expansion replaces arguments of the form "@file" with the contents of the file.

## Conversions

Static member functions | &nbsp;
---------|------------
Cli::toArgv(string) | Parse command line into argument vector of strings, using default conventions (Gnu or Windows) of the platform.
Cli::toArgv(argc, argv) | Copy array of pointers into argument vector of strings.
Cli::toGlibArgv(string) | Parse according to glib conventions, based on the UNIX98 shell spec.
Cli::toGnuArgv(string) | Parse using GNU conventions, same rules as buildargv().
Cli::toWindowsArgv(string) | Parse using Windows rules.
Cli::toCmdline | Join arguments into a single command line, escaping as needed, that will parse back into those same arguments. Uses the default conventions (Gnu or Windows).
Cli::toGlibCmdline | Join according to glib conventions, based on UNIX98 shell spec.
Cli::toGnuCmdline | Join using GNU conventions, same rules as buildargv()
Cli::toWindowsCmdline | Join using Windows rules.
<b>Member functions</b> |
cli.fromString&lt;T> | Parses string into any supported type.
cli.toString&lt;T> | Converts value of any supported type into a string.
