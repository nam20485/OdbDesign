/**
 * @name HTTP Response Splitting
 * @description Detects potential HTTP response splitting vulnerabilities in web server code
 * @kind path-problem  
 * @problem.severity error
 * @precision high
 * @id cpp/odbdesign-http-response-splitting
 * @tags security
 *       external/cwe/cwe-113
 *       odbdesign
 *       web-server
 */

import cpp
import semmle.code.cpp.dataflow.TaintTracking
import DataFlow::PathGraph

/**
 * A taint configuration for HTTP response splitting vulnerabilities.
 */
class HttpResponseSplittingConfig extends TaintTracking::Configuration {
  HttpResponseSplittingConfig() { this = "HttpResponseSplittingConfig" }

  override predicate isSource(DataFlow::Node source) {
    // HTTP request parameters/headers that could be user-controlled
    exists(FunctionCall fc, Function f |
      f = fc.getTarget() and
      (
        // Crow framework request methods
        f.hasQualifiedName("crow", "request", "get_header_value") or
        f.hasQualifiedName("crow", "request", "url_params") or
        // General HTTP input functions
        f.getName().matches("%get_param%") or
        f.getName().matches("%header%") or
        f.getName().matches("%query%")
      ) and
      source.asExpr() = fc
    )
  }

  override predicate isSink(DataFlow::Node sink) {
    // HTTP response writing functions
    exists(FunctionCall fc, Function f |
      f = fc.getTarget() and
      (
        // Crow response methods
        f.hasQualifiedName("crow", "response", "write") or
        f.hasQualifiedName("crow", "response", "set_header") or
        // General response writing
        f.getName().matches("%write%") or
        f.getName().matches("%send%") or
        f.getName().matches("%response%")
      ) and
      sink.asExpr() = fc.getAnArgument()
    )
  }

  override predicate isSanitizer(DataFlow::Node node) {
    // Consider URL encoding/escaping as sanitization
    exists(FunctionCall fc, Function f |
      f = fc.getTarget() and
      (
        f.getName().matches("%encode%") or
        f.getName().matches("%escape%") or
        f.getName().matches("%sanitize%")
      ) and
      node.asExpr() = fc
    )
  }
}

from HttpResponseSplittingConfig config, DataFlow::PathNode source, DataFlow::PathNode sink
where 
  config.hasFlowPath(source, sink) and
  // Focus on OdbDesignServer directory
  source.getNode().getLocation().getFile().getAbsolutePath().matches("%OdbDesignServer%")
select sink.getNode(), source, sink,
  "Potential HTTP response splitting: user input from $@ flows to HTTP response without proper validation.",
  source.getNode(), "user input"
