#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#include "lbl/factored_nlm.h"
#include "lbl/model_utils.h"

using namespace boost::program_options;
using namespace oxlm;

int main(int argc, char** argv) {
  options_description desc("General options");
  desc.add_options()
      ("help,h", "Print help message")
      ("model,m", value<string>()->required(), "File containing the model")
      ("contexts,c", value<string>()->required(),
          "File containing the contexts");

  variables_map vm;
  store(parse_command_line(argc, argv, desc), vm);

  if (vm.count("help")) {
    cout << desc << endl;
    return 0;
  }

  notify(vm);

  boost::shared_ptr<FactoredNLM> model = loadModel(
      vm["model"].as<string>(), boost::shared_ptr<Corpus>());

  string line;
  ifstream in(vm["contexts"].as<string>());
  while (getline(in, line)) {
    istringstream sin(line);
    string word;
    vector<int> context;
    double sum = 0;
    while (sin >> word) {
      context.push_back(model->label_id(word));
    }

    for (int word_id = 0; word_id < model->labels(); ++word_id) {
      cout << model->label_str(word_id) << " ";
      for (int context_word: context) {
        cout << model->label_str(context_word) << " ";
      }
      double prob = exp(model->log_prob(word_id, context, true, true));
      cout << prob << endl;
      sum += prob;
    }

    cout << "====================" << endl;
    assert(fabs(1 - sum) < 1e-5);
  }

  return 0;
}
