import Module from "../generated/hololisp.js";

class HllVm {
    constructor(hll_make_vm, hll_interpret, hll_delete_vm) {
        this.hll_make_vm = hll_make_vm;
        this.hll_interpret = hll_interpret;
        this.hll_delete_vm = hll_delete_vm;

        this.vm = this.hll_make_vm(0);
    }

    interpret(code) {
        this.hll_interpret(this.vm, "wasm", code, false);
    }
}

let vm = null;

const handle_print = (text) => {
    document.getElementById("editor-output-code").value += text + "\n";
};

window.onload = (_) => {
    Module({
        print: handle_print
    }).then((Module) => {
        Module["print"] = (text) => {
            console.log("hello)))))))", text)
        };
        const hll_make_vm = Module.cwrap("hll_make_vm", "number", ["number"]);
        const hll_interpret = Module.cwrap("hll_interpret", "number", ["number", "string", "string", "number"]);
        const hll_delete_vm = Module.cwrap("hll_delete_vm", null, ["number"]);
        vm = new HllVm(hll_make_vm, hll_interpret, hll_delete_vm);
    }).catch((err) => console.error("Fatal error: Failed to initialize wasm", err))

    document.getElementById("run").onclick = () => {
        document.getElementById("editor-output-code").value = "";
        const code = document.getElementById("editor-playground-code").value;
        vm.interpret(code);
        console.log("executed");
    }
};
