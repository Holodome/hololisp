<script>
  const default_code = ";; You can edit this code!\n" +
      ";; Click here and start typing\n" +
      "(print 'hello-world)";

  import { vm } from "../hll_vm";

  console.log(vm);
  vm.interpret("(print 'playground)");
</script>

<div class="lined-playground">
  <textarea id="code" class="playground" autocomplete="off" spellcheck="false">{default_code}
  </textarea>
</div>

<style>
    .lined-playground {
        height: 100%;
        overflow: hidden;
        width: 100%;
        padding: 0;
        font-size: 0.8rem;
        box-sizing: border-box;
        resize: vertical;
        margin: 0;
    }

    .playground {
        width: 100%;
        height: 100%;
        /*white-space: nowrap;*/
        background: #1e1e1e;
        border: none;
        outline: none;
        resize: none;
        color: inherit;
        float: right;
    }
</style>