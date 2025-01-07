module.exports = {
    mainTemplate: "{{#each commitGroups}}{{#if title}}<p>{{title}}:</p><ul>{{#each commits}}{{> commit root=@root}}{{/each}}</ul>{{/if}}{{/each}}",
    commitPartial: "<li>{{#if subject}}{{subject}}{{else}}{{header}}{{/if}}</li>",
    transform: (commit) => {
        let type = commit.type;

        if (commit.type === `feat`) {
            type = `Features`;
        } else if (commit.type === `fix`) {
            type = `Bug Fixes`;
        } else if (commit.type === `perf`) {
            type = `Performance Improvements`;
        } else if (commit.type === `revert`) {
            type = `Reverts`;
        } else if (commit.type === `docs`) {
            type = `Documentation`;
        } else if (commit.type === `style`) {
            type = `Styles`;
        } else if (commit.type === `refactor`) {
            type = `Code Refactoring`;
        } else if (commit.type === `test`) {
            type = `Tests`;
        } else if (commit.type === `build`) {
            type = `Build System`;
        } else if (commit.type === `chore`) {
            type = `Maintenance`;
        } else if (commit.type === `ci`) {
            type = `Continuous Integration`;
        }

        return {
            type
        };
    }
}