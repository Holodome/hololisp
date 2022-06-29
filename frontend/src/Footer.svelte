<script>
  import DiGithubBadge from "svelte-icons/di/DiGithubBadge.svelte";
</script>

<main>
  <footer>
    <div class="github-icon">
      <span class="github-badge">
        <DiGithubBadge/>
      </span>
      GitHub:
      <a href="https://github.com/holodome" class="github-link">
        Holodome
      </a>
    </div>
  </footer>
</main>

<style>
  footer {
      margin-top: auto;
      position: fixed;
      bottom: 0;
      left: 0;
      right: 0;
      height: 3em;
      padding: 0 .6em;
      box-shadow: 0 0 .2em rgba(170,170,170,.2);
      font-size: 90%;
  }

  .github-badge {
      display: inline-block;
      position: relative;
      top: 0;
      width: 1.5em;
      height: 1.5em;
      transform: translateY(25%);
  }

  .github-icon {
      vertical-align: bottom;
      position: relative;
      text-align: center;
      transform: translateY(25%);
      color: dimgray;
  }

  .github-link {
      color: #EEE;
      transition: .1s color ease;
  }

  .github-link:hover {
      color: #1370bf;
  }
</style>