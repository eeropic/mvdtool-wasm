import JSON5 from './json5.min.mjs'
import createMvdToolModule from '../output-web/mvdtool.js'

async function initializeWasm() {
    const moduleOverrides = {
        print(text) {
            // console.log("Output:", text);
        },
        printErr(text) {
            console.error("Error:", text);
        },
        onRuntimeInitialized() {
            console.log("WASM Loaded");
            Module.runMvdtool = Module.cwrap("main", "number", ["number", "array"]);
        },
        noInitialRun: true
    };

    const Module = await createMvdToolModule(moduleOverrides);

    return Module;
}

async function runMvdtool() {
    const Module = await initializeWasm();

    const fileInput = document.getElementById("fileInput");
    const file = fileInput.files[0];

    if (!file) {
        alert("Please select a file first.");
        return;
    }

    const reader = new FileReader();
    reader.onload = function (event) {
        const data = new Uint8Array(event.target.result);
        Module.FS_createDataFile("/", "demo.mvd2", data, true, true);

        const outputFilename = "output.txt";
        const args = ["mvdtool", "bin2json", "demo.mvd2", outputFilename];

        const argv = Module._malloc(args.length * 4);
        const argvPointers = [];

        for (let i = 0; i < args.length; i++) {
            const argPtr = Module._malloc(Module.lengthBytesUTF8(args[i]) + 1);
            Module.stringToUTF8(args[i], argPtr, Module.lengthBytesUTF8(args[i]) + 1);
            Module.setValue(argv + i * 4, argPtr, "i32");
            argvPointers.push(argPtr);
        }

        Module.ccall("main", "number", ["number", "number"], [args.length, argv]);

        try {
            const output = Module.FS_readFile(outputFilename, { encoding: "utf8" });
            // console.log("Output File Content:", output);
            document.getElementById('output').innerText = output
        } catch (err) {
            console.error("Error reading output file:", err);
        }

        for (let i = 0; i < argvPointers.length; i++) {
            Module._free(argvPointers[i]);
        }
        Module._free(argv);
    };

    reader.readAsArrayBuffer(file);
}

document.getElementById('run-button').addEventListener('click', runMvdtool);