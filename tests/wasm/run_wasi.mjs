// Runs a wasm32-wasi module with Node's WASI, on this process's stdin,
// stdout and stderr, and exits with the module's exit code.
// Usage: node --no-warnings run_wasi.mjs <module.wasm>
import { readFile } from 'node:fs/promises';
import { WASI } from 'node:wasi';

const wasi = new WASI({ version: 'preview1', args: [process.argv[2]], returnOnExit: true });
const module = await WebAssembly.compile(await readFile(process.argv[2]));
const instance = await WebAssembly.instantiate(module, wasi.getImportObject());
process.exitCode = wasi.start(instance);
