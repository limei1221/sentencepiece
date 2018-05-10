// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.!

#include "word_model_trainer.h"

#include "builder.h"
#include "sentencepiece_processor.h"
#include "testharness.h"
#include "util.h"

namespace sentencepiece {
namespace word {
namespace {

// Space symbol (U+2581)
#define WS "\xE2\x96\x81"

std::string RunTrainer(const std::vector<std::string> &input, int size) {
  test::ScopedTempFile input_scoped_file("input");
  test::ScopedTempFile model_scoped_file("model");
  const std::string input_file = input_scoped_file.filename();
  const std::string model_prefix = model_scoped_file.filename();
  {
    io::OutputBuffer output(input_file);
    for (const auto &line : input) {
      output.WriteLine(line);
    }
  }

  TrainerSpec trainer_spec;
  trainer_spec.set_model_type(TrainerSpec::WORD);
  trainer_spec.add_input(input_file);
  trainer_spec.set_vocab_size(size - 3);  // remove <unk>, <s>, </s>
  trainer_spec.set_model_prefix(model_prefix);

  NormalizerSpec normalizer_spec;
  normalizer_spec.set_name("identity");
  EXPECT_OK(normalizer::Builder::PopulateNormalizationSpec(&normalizer_spec));
  normalizer_spec.set_add_dummy_prefix(true);

  Trainer trainer(trainer_spec, normalizer_spec);
  EXPECT_OK(trainer.Train());

  SentencePieceProcessor processor;
  EXPECT_OK(processor.Load(model_prefix + ".model"));

  const auto &model = processor.model_proto();
  std::vector<std::string> pieces;

  // remove <unk>, <s>, </s>
  for (int i = 3; i < model.pieces_size(); ++i) {
    pieces.emplace_back(model.pieces(i).piece());
  }

  return string_util::Join(pieces, " ");
}
}  // namespace

TEST(TrainerTest, BasicTest) {
  EXPECT_EQ(WS "I " WS "apple " WS "have " WS "pen",
            RunTrainer({"I have a pen", "I have an apple", "apple pen"}, 10));
}
}  // namespace word
}  // namespace sentencepiece
