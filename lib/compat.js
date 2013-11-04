
function require(module)
{
  return require.scopes[module];
}
require.scopes = {__proto__: null};
