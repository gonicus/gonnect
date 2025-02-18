module.exports = {
    mainTemplate: "{{#each commitGroups}}{{#if title}}<p>{{title}}:</p><ul>{{#each commits}}{{> commit root=@root}}{{/each}}</ul>{{/if}}{{/each}}",
    commitPartial: "<li>{{#if subject}}{{subject}}{{else}}{{header}}{{/if}}</li>",
    transform: (commit) => {
        let type = commit.type;

        if (commit.type === `feat`) {
            type = `Features`;
        } else if (commit.type === `fix`) {
            type = `Bug Fixes`;
        } else {
            type = ``;
        }

        return {
            type
        };
    }
}