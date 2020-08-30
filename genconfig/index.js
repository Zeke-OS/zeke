const vm = require('vm');
const { readFileSync, promises: fs } = require('fs');
const Ajv = require('ajv');

const schema = JSON.parse(readFileSync(process.argv[2]));
const ajv = new Ajv({
    loadSchema: loadSchema,
    removeAdditional: 'all',
    useDefaults: true,
    verbose: true,
    allErrors: true,
    extendRefs: true,
    strictDefaults: true,
    strictKeywords: true,
    strictNumbers: true,
});
const configKnobMap = {};
const configMap = {};
const dependencies = [];
const selects = [];

async function loadSchema(uri) {
    return JSON.parse(await fs.readFile(uri));
}

function errorPathToPath(epath) {
    return epath.substring(2, epath.length - 1);
}

ajv.addKeyword('types', {
    type: 'object'
});
ajv.addKeyword('metaType', {
    macro: (type, parentSchema, obj) => {
        if (type === 'menu') {
            return {
                type: 'object'
            };
        } else if (type === 'menuconfig') {
            return {
                type: 'object',
                required: ['enabled']
            };
        }

        const choice = parentSchema.choice;
        if (type === 'boolChoice' && choice) {
            for (const k in choice) {
                configKnobMap[choice[k]] = {
                    path: errorPathToPath(obj.errorPath),
                    type: parentSchema.metaType,
                    makeOnly: parentSchema.makeOnly
                };
            }

            return {
                type: 'string',
                enum: Object.keys(choice)
            }
        }

        if (!parentSchema.config) {
            return {
                allOf: [
                    {},
                    { not: {} }
                ]
            }
        }

        if (type === 'int') {
            return {
                type: 'integer',
            };
        } else if (type === 'str') {
            return {
                type: 'string',
            }
        } else if (type === 'bool') {
            return {
                type: 'boolean',
            };
        } else if (type === 'tristate') {
            return {
                type: 'string',
                pattern: '^[nmy]$',
            };
        } else {
            return {
                allOf: [
                    {},
                    { not: {} }
                ]
            }
        }
    },
    metaSchema: {
        type: 'string'
    }
});
ajv.addKeyword('choice', {
    type: 'string',
    validate: (choice, value, parentSchema) => {
        if (!parentSchema.metaType) {
            return false;
        };

        configMap[choice[value]] = true;

        return true;
    },
    metaSchema: {
        type: 'object',
        patternProperties: {
            '^.*$': {
                type: 'string'
            }
        },
        additionalProperties: false
    }
});
ajv.addKeyword('makeOnly', {
    dependencies: ['config'],
    metaSchema: {
        type: 'boolean'
    }
});
ajv.addKeyword('config', {
    dependencies: ['default', 'metaType'],
    compile: (name, parentSchema, obj) => {
        configKnobMap[name] = {
            path: errorPathToPath(obj.errorPath),
            type: parentSchema.metaType,
            makeOnly: parentSchema.makeOnly
        };

        // TODO Validate that config is under correct type of object
        return (value, dataPath) => {
            if (configMap[name]) {
                return false;
                // TODO Add error
            }

            if (!parentSchema.metaType) {
                return false;
            };

            configMap[name] = true;
            return true;
        };
    },
    metaSchema: {
        type: 'string'
    }
});
ajv.addKeyword('depends', {
    dependencies: ['config'],
    validate: (...args) => {
        dependencies.push([args[2].config, args[0]]);
        return true;
    },
    metaSchema: {
        type: 'string'
    }
});
ajv.addKeyword('select', {
    //dependencies: ['config'],
    modifying: true,
    validate: (...args) => {
        let sel = args[0];
        const val = args[1];
        const data = args[6];

        // TODO Remove
        console.log('val', val);
        if (!val) return true;

        if (typeof sel === 'string') {
            sel = [{ knob: sel, expression: 'true' }];
        } else if (Array.isArray(sel) && typeof sel[0] === 'string') {
            sel = sel.map((s) => ({ knob: sel, expression: 'true' }));
        } else if (!Array.isArray(sel)) {
            sel = [sel];
        }

        for (const { knob, expression } of sel) {
            if (evalExpression(data, expression)) {
                const { path } = configKnobMap[knob];
                setBool(data, path);
            }
        }

        return true;
    },
    metaSchema: {
        oneOf: [
            { type: 'string' },
            {
                type: 'array',
                uniqueItems: true,
                items: { type: 'string' }
            },
            {
                type: 'object',
                properties: {
                    knob: { type: 'string' },
                    expression: { type: 'string' }
                },
                additionalProperties: false
            },
            {
                type: 'array',
                uniqueItems: true,
                items: {
                    type: 'object',
                    properties: {
                        knob: { type: 'string' },
                        expression: { type: 'string' }
                    },
                    additionalProperties: false
                }
            }
        ],
        additionalProperties: false
    }
});

function getValue(obj, path) {
    path = path.split('.');
    for (let i = 0; i < path.length; i++) {
        obj = obj[path[i]];
        if (!obj) break;
    };
    return obj;
};

function setBool(obj, path) {
    path = path.split('.');
    let i;
    for (i = 0; i < path.length - 1; i++) {
        const tmp = obj[path[i]];
        if (!tmp) {
            tmp = {};
            obj[path[i]] = tmp;
        };
        obj = tmp;
    };

    obj[path[i]] = true;
}

function prepareSandbox(data) {
    const sandbox = {};
    for (const name in configKnobMap) {
        const { path, type } = configKnobMap[name];

        sandbox[name] = {
            value: getValue(data, path),
            type
        };
    }

    // Parse values into variables that will work nicely for simple expressions.
    for (k in sandbox) {
        const o = sandbox[k];
        if (!o) continue;
        const value = o.value;
        const type = o.type;

        if (type === 'tristate') {
            sandbox[k] = value === 'n' ? false : value;
        } else {
            sandbox[k] = value;
        }
    }

    return sandbox;
}

function evalExpression(data, expression) {
    return !!vm.runInNewContext(expression, prepareSandbox(data));
}

function knob2Makefile(name, type, value) {
    if ('str' === type) {
        return `${name}="${value}"`
    } else if (['bool', 'boolChoice'].includes(type)) {
        return `${name}=${value ? 'y' : 'n'}`;
    }
    return `${name}=${value}`;
}

function knob2C(name, type, value) {
    if ('str' === type) {
        return `#define ${name} "${value}"`
    } else if (['bool', 'boolChoice'].includes(type)) {
        if (!value) return null;
        return `#define ${name} 1`;
    } else if (type === 'tristate') {
        let tri;

        switch (value) {
            case 'n': return null;
            case 'y': tri = 1; break;
            case 'm': tri = 2; break;
        }
        return `#define ${name} ${tri}`;
    }
    return `#define ${name} ${value}`;
}

const config = JSON.parse(readFileSync(process.argv[3]));
ajv.compileAsync(schema).then(function (validate) {
    console.log('Knobs\n=====\n', Object.keys(configKnobMap), '\n');

    const valid = validate(config);

    if (!valid) {
        console.log(JSON.stringify(validate.errors, null, 2));
        process.exit(1);
    }

    let passed = true;
    for (const dependency of dependencies) {
        const [name, expression] = dependency;
        if (!evalExpression(config, `!${name} || (${expression})`)) {
            console.log(`"${name}" depends on: "${expression}"`);
            passed = false;
        }
    }
    if (!passed) {
        process.exit(1);
    }

    console.log('js\n==\n', JSON.stringify(config, null, 2), '\n');

    console.log('Makefile\n========');
    for (const k in configMap) {
        const { path, type } = configKnobMap[k];
        const value = getValue(config, path);
        console.log(knob2Makefile(k, type, value));
    }

    console.log('\nC\n=');
    for (const k in configMap) {
        const { path, type, makeOnly } = configKnobMap[k];
        const value = getValue(config, path);
        if (makeOnly) continue;
        const line = knob2C(k, type, value);
        if (line) console.log(line);
    }
}).catch(console.error);
