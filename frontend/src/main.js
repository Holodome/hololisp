import Module from "../generated/hololisp.js";
import {Examples} from "../generated/examples.js";

class HllVm {
    constructor(hll_make_vm, hll_interpret, hll_delete_vm) {
        this.hll_make_vm = hll_make_vm;
        this.hll_interpret = hll_interpret;
        this.hll_delete_vm = hll_delete_vm;

        this.vm = this.hll_make_vm(0);
    }

    interpret(code) {
        this.hll_interpret(this.vm, code, "wasm", 0);
    }
}

let vm = null;

const handle_print = (text) => {
    document.getElementById("editor-output-code").value += text + "\n";
};

window.onload = (_) => {
    Module({
        print: handle_print, printErr: handle_print
    }).then((Module) => {
        const hll_make_vm = Module.cwrap("hll_make_vm", "number", ["number"]);
        const hll_interpret = Module.cwrap("hll_interpret", "number", ["number", "string", "string", "number"]);
        const hll_delete_vm = Module.cwrap("hll_delete_vm", null, ["number"]);
        vm = new HllVm(hll_make_vm, hll_interpret, hll_delete_vm);
    }).catch((err) => {
        console.error(err);
        alert("Fatal error: Failed to initialize wasm")
    });

    let examples_select = document.getElementById("examples");
    examples_select.innerHTML = Object.keys(Examples).map(it => "<option value=\""
        + it + "\">" + it + "</option>").join(" ");
    examples_select.onchange = (e) => {
        console.log("as");
        document.getElementById("editor-playground-code").value = Examples[e.target.value];
    };

    document.getElementById("editor-playground-code").value = Examples["hello world"];

    document.getElementById("run").onclick = () => {
        document.getElementById("editor-output-code").value = "";
        const code = document.getElementById("editor-playground-code").value;
        vm.interpret(code);
    };
};
