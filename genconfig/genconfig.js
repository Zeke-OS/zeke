const { readSchemaFile, readConfigFile, parseConfig } = require('./src/gen');

const schemaFile = process.argv[2];
const configFile = process.argv[3];
const {
    knobs,
    config,
    makefile,
    header
} = parseConfig(readSchemaFile(schemaFile), readConfigFile(configFile));

console.log('Knobs\n====');
console.log(JSON.stringify(knobs, null, 2));

console.log('\njs\n==');
console.log(JSON.stringify(config, null, 2));

console.log('\nMakefile\n========');
console.log(makefile);

console.log('\nC header\n========');
console.log(header);
