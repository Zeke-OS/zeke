const blessed = require('blessed');

module.exports = function createHelpBox(parent, schema) {
    const helpBox = blessed.box({
      parent,
      label: 'Description',
      top: 0,
      width: '100%',
      height: 5,
      border: {
          type: 'line'
      },
      style: {
          border: {
              fg: 'white'
          }
      }
    });
    const helpText = blessed.text({ parent: helpBox });
    helpText.setText(schema.description || 'No help text')

    return helpBox;
}
