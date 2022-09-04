import Module from "../dist/hololisp.js";
import {HllVm} from "./hll_vm";

export class HllVm {
    readonly vm: object;

    readonly hll_make_vm: Function;
    readonly hll_interpret: Function;
    readonly hll_delete_vm: Function

    constructor(Module: { cwrap: Function }) {
        this.hll_make_vm = Module.cwrap("hll_make_vm", "number", ["number"]);
        this.hll_interpret = Module.cwrap("hll_interpret", "number", ["number", "string", "string", "number"]);
        this.hll_delete_vm = Module.cwrap("hll_delete_vm", null, ["number"]);

        this.vm = this.hll_make_vm(0);
    }

    interpret(code: string) {
        this.hll_interpret(this.vm, "wasm", code, false);
    }
}

export let vm;

Module().then((Module) => {
    console.log("created");
    vm = new HllVm(Module);
    vm.interpret("(print (+ 1 2))");
}).catch((err) => console.error(err));