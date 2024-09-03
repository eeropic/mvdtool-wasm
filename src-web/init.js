Module = {
  print: function (text) {
    console.log("Output:", text)
  },
  printErr: function (text) {
    console.error("Error:", text)
  },
  onRuntimeInitialized: function () {
    console.log("WASM Loaded")
    Module.runMvdtool = Module.cwrap("main", "number", [
      "number",
      "array",
    ])
  },
  noInitialRun: true,
}