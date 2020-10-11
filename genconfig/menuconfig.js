const blessed = require('blessed');
const createMenu = require('./src/menu');
const { readSchemaFile, readConfigFile, parseConfig } = require('./src/gen');

const schemaFile = process.argv[2];
const configFile = process.argv[3];
const schema = readSchemaFile(schemaFile);
const {
    knobs,
    config,
    makefile,
    header
} = parseConfig(schema, readConfigFile(configFile));

// Create a screen object.
const screen = blessed.screen({
  smartCSR: true
});

screen.title = 'Zeke Config';
const menu = createMenu(screen, schema, config);

// Quit on Escape, q, or Control-C.
screen.key(['q', 'C-c'], function(ch, key) {
  return process.exit(0);
});

// Focus our element.
menu.focus();

// Render the screen.
screen.render();
