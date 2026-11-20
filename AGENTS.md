# AGENTS.md

nnam is an Asuka app for exporting audio and tags from NCM files.

Use the [documentation index](docs/README.md) to find architecture and testing
guides. Read the [architecture](docs/architecture.md) before changing module
boundaries or the import and export flow.

1. Before editing `.asuka` files, run `asuka guide` for the installed compiler.
2. For source changes, run `asuka check --json`, `asuka test --json` and
   `asuka test --json converter` from the project root. Asuka runs tests
   by package. Use `asuka explain <code>` to look up a diagnostic.

Keep component bodies declarative. Update state in handlers, functions, tasks
or effects, and give changing view lists a `key`.

Keep AES-128-ECB in C and NCM parsing, audio decryption and tag writing in
Asuka. Keep file I/O and decryption in `NcmConverter`'s worker. Write exports
to a temporary file before renaming them into place.

For changes to NCM parsing, tags or export, use the relevant regression tests
in the [development guide](docs/development.md#回归测试).

When behavior, architecture or verification changes, update the relevant
documentation in the same change.
