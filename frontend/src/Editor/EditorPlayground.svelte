<script>
    import { query_selector_all, transition_in } from "svelte/internal";
    const default_code =
        ";; You can edit this code!\n" +
        ";; Click here and start typing\n" +
        "(print 'hello-world)";

    function count_lines() {
        let count = 0;
        const a = document.querySelector("textarea");
        for (let i = 0; i < a.value.length; i++) {
            if (a.value[i] == "\n") {
                count++;
            }
        }
        return count;
    }

    function add_number_lines() {
        var lines = count_lines();

        const deleteElement = document.getElementById("lines");
        deleteElement.innerHTML = "";

        for (let i = 1; i < lines + 2; i++) {
            var activeDiv = document.getElementById("lines");
            var elem = document.createElement("span");
            elem.style.cssText =
                "border-left: 1px solid yellow;text-align: right;display: block;margin-right: .4rem; font-size: 1rem;";
            // @ts-ignore
            var elemText = document.createTextNode(i);
            elem.appendChild(elemText);
            activeDiv.appendChild(elem);
        }
    }
</script>

<div class="lined-playground">
    <div id="lines" class="lines-number">
        <span class="line-number">1</span>
        <span class="line-number">2</span>
        <span class="line-number">3</span>
    </div>
    <form class="form" name="form">
        <textarea
            id="code"
            class="playground"
            autocomplete="off"
            rows="45"
            spellcheck="false"
            on:keyup={add_number_lines}
            >{default_code}
        </textarea>
    </form>
</div>

<style>
    .form {
        width: 100%;
        padding: 0;
    }
    .lined-playground {
        display: flex;
        height: 100%;
        overflow: hidden;
        width: 100%;
        padding: 0;
        font-size: 18px;
        box-sizing: border-box;
        resize: vertical;
        margin: 0;
    }
    .lines-number {
        min-width: 3%;
    }
    .line-number {
        display: block;
        font-size: 1rem;
        margin-right: .4rem;
        text-align: right;
        border-left: 1px solid yellow;
        font-family: sans-serif;
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
        box-sizing: border-box;
        font-size: 1rem;
        padding: 0 0 0 .4rem;
        font-family: sans-serif;
    }
</style>
