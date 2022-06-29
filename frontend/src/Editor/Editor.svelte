<script>
  import EditorControls from "./EditorControls.svelte";
  import EditorPlayground from "./EditorPlayground.svelte";
  import EditorOutput from "./EditorOutput.svelte";
</script>

<main>
  <EditorControls/>
  <EditorPlayground/>
  <EditorOutput/>
</main>

<style>
  main {
      display: flex;
      text-align: left;
      width: 90%;
      max-width: 960px;
      margin: 0 auto;
      padding: 20px 0 0;
      flex-wrap: wrap;
  }
</style>