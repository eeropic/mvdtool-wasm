function runMvdtool() {
  const fileInput = document.getElementById("fileInput")
  const file = fileInput.files[0]

  if (!file) {
    alert("Please select a file first.")
    return
  }

  const reader = new FileReader()
  reader.onload = function (event) {
    const data = new Uint8Array(event.target.result)
    Module.FS_createDataFile("/", "demo.mvd2", data, true, true)

    const args = ["mvdtool", "strings", "demo.mvd2"]

    const argv = Module._malloc(args.length * 4)
    const argvPointers = []

    for (var i = 0; i < args.length; i++) {
      const argPtr = Module._malloc(Module.lengthBytesUTF8(args[i]) + 1)
      Module.stringToUTF8(
        args[i],
        argPtr,
        Module.lengthBytesUTF8(args[i]) + 1
      )
      Module.setValue(argv + i * 4, argPtr, "i32")
      argvPointers.push(argPtr)
    }

    Module.ccall(
      "main",
      "number",
      ["number", "number"],
      [args.length, argv]
    )

    for (var i = 0; i < argvPointers.length; i++) {
      Module._free(argvPointers[i])
    }
    Module._free(argv)
  }

  reader.readAsArrayBuffer(file)
}