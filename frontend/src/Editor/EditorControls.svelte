<script>
  const examples = [
      "conway's game of life"
  ];
</script>

<div class="controls">
  <h3>hololisp playground</h3>
  <span class="controls-buttons">
    <button id="run" class="controls-button-run">Run</button>
    <button id="format">Format</button>
    <select>
      {#each examples as example}
        <option>{example}</option>
      {/each}
    </select>
  </span>
</div>

<style>
  .controls {
      top: 0;
      left: 0;
      right: 0;
      width: 100%;
  }

  .controls h3 {
      float: left;
      position: relative;
  }

  .controls-buttons {
      float: right;
      right: 0;
      position: relative;
      top: 25%;
  }

  .controls button, .controls select {
      font: inherit;
      display: inline-block;
      text-align: center;
      border: 0;
      width: auto;
      height: auto;
      vertical-align: baseline;
      cursor: pointer;
      transition: all .3s;
      border-radius: 0.2em;
      padding: .2em .7em;
  }

  .controls-button-run {
    background: #0074d9;
  }

</style>