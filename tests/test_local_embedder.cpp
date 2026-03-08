#include "local_embedder.hpp"
#include "vector_db.hpp"

#include <boost/asio/thread_pool.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstddef>
#include <vector>

using namespace muclaw;

TEST_CASE("LocalEmbedder tests", "[local_embedder]") {
    boost::asio::thread_pool pool{1};
    muclaw::VectorDB db{pool, 384};
    muclaw::LocalEmbedder::init();

    auto& model = LocalEmbedder::instance();

    SECTION("Embeddings are non-NaN and non-zero") {
        auto embedding = model.embed("The stock market crashed today due to unexpected inflation data.");
        bool has_non_zero = false;
        for (auto x : embedding) {
            REQUIRE_FALSE(std::isnan(x));
            if (x != 0.0f)
                has_non_zero = true;
        }
        REQUIRE(has_non_zero);
    }

    SECTION("Correct embeddings for built-in model") {
        // expected_embedding assumes builtin model
        if (model.model_path.filename() == "all-MiniLM-L6-v2-f16.gguf") {
            auto result = model.embed("hello world");
            REQUIRE(result.size() == 384);
            std::vector<float> expected_embedding = {
                -0.034479f, 0.030934f,  0.006650f,  0.026090f,  -0.039363f, -0.160436f, 0.066865f,  -0.006561f,
                -0.047535f, 0.014787f,  0.070949f,  0.055473f,  0.019195f,  -0.026226f, -0.009964f, -0.026972f,
                0.022335f,  -0.022185f, -0.149766f, -0.017529f, 0.007581f,  0.054262f,  0.003251f,  0.031736f,
                -0.084664f, -0.029335f, 0.051529f,  0.048145f,  -0.003282f, -0.058207f, 0.041941f,  0.022228f,
                0.128135f,  -0.022308f, -0.011722f, 0.062988f,  -0.032824f, -0.091266f, -0.031163f, 0.052734f,
                0.047129f,  -0.084141f, -0.029980f, -0.020727f, 0.009426f,  -0.003580f, 0.007385f,  0.039336f,
                0.093251f,  -0.003709f, -0.052675f, -0.058084f, -0.006896f, 0.005218f,  0.082878f,  0.019277f,
                0.006299f,  -0.010309f, 0.008905f,  -0.037719f, -0.045166f, 0.023876f,  -0.006903f, 0.013467f,
                0.100099f,  -0.071564f, -0.021664f, 0.031674f,  -0.051626f, -0.082252f, -0.065794f, -0.009835f,
                0.005760f,  0.073655f,  -0.034039f, 0.024873f,  0.014491f,  0.026454f,  0.009646f,  0.030269f,
                0.052888f,  -0.075346f, 0.009828f,  0.029884f,  0.017569f,  0.023125f,  0.001909f,  0.001326f,
                -0.047189f, -0.011273f, -0.114246f, -0.020028f, 0.040307f,  0.002291f,  -0.079855f, -0.025314f,
                0.094579f,  -0.029065f, -0.144948f, 0.230943f,  0.027719f,  0.032076f,  0.031069f,  0.042891f,
                0.064268f,  0.032119f,  -0.004894f, 0.055805f,  -0.037597f, -0.021479f, -0.028459f, -0.028904f,
                0.038372f,  -0.017426f, 0.052508f,  -0.074945f, -0.031152f, 0.021927f,  -0.039802f, -0.008691f,
                0.026967f,  -0.048582f, 0.011381f,  0.029659f,  -0.020581f, 0.013144f,  0.028826f,  -0.000000f,
                0.064798f,  -0.018096f, 0.051918f,  0.121913f,  0.028768f,  0.008810f,  -0.070412f, -0.016816f,
                0.040711f,  0.042220f,  0.025414f,  0.035791f,  -0.049079f, 0.002147f,  -0.015494f, 0.050619f,
                -0.048119f, 0.035878f,  -0.004201f, 0.101710f,  -0.055939f, -0.010641f, 0.011222f,  0.090656f,
                0.004332f,  0.035101f,  -0.009674f, -0.093844f, 0.092796f,  0.007986f,  -0.007627f, -0.052160f,
                -0.012571f, 0.003192f,  0.005996f,  0.007500f,  0.010525f,  -0.086335f, -0.069874f, -0.002541f,
                -0.091111f, 0.046860f,  0.052046f,  0.007223f,  0.010924f,  -0.005304f, 0.013829f,  0.021907f,
                0.034093f,  0.060317f,  0.000146f,  0.014649f,  -0.070038f, 0.028427f,  -0.027519f, 0.010852f,
                0.034970f,  -0.022417f, 0.009672f,  0.077266f,  0.021672f,  0.114870f,  -0.068044f, 0.023921f,
                -0.015995f, -0.017761f, 0.064427f,  0.032024f,  0.050258f,  -0.005980f, -0.033790f, 0.017862f,
                0.016594f,  0.063431f,  0.034760f,  0.046584f,  0.097893f,  -0.006585f, 0.025069f,  -0.077835f,
                0.016902f,  -0.000955f, 0.022559f,  -0.038266f, 0.095678f,  -0.005356f, 0.010555f,  -0.115324f,
                -0.013259f, -0.010766f, -0.083145f, 0.073231f,  0.049435f,  -0.009012f, -0.095775f, 0.000000f,
                0.124899f,  0.019196f,  -0.058172f, -0.035957f, -0.050805f, -0.045683f, -0.082643f, 0.148172f,
                -0.088326f, 0.060329f,  0.051058f,  0.010311f,  0.141158f,  0.030859f,  0.061025f,  -0.052831f,
                0.136648f,  0.009164f,  -0.017284f, -0.012848f, -0.007860f, -0.051060f, -0.052316f, 0.007689f,
                -0.015192f, 0.017016f,  0.021339f,  0.020500f,  -0.120024f, 0.014594f,  0.026718f,  0.025276f,
                -0.042710f, 0.006802f,  -0.014459f, 0.045118f,  -0.091404f, -0.019474f, -0.017809f, -0.055012f,
                -0.052711f, -0.010339f, -0.052066f, 0.020941f,  -0.080030f, -0.012138f, -0.057816f, 0.023239f,
                -0.007866f, -0.025786f, -0.079856f, -0.020663f, 0.048921f,  -0.020465f, -0.049193f, 0.014077f,
                -0.063751f, -0.007839f, 0.016429f,  -0.025721f, 0.013299f,  0.026208f,  0.009867f,  0.063115f,
                0.002548f,  -0.006604f, 0.016657f,  0.032434f,  0.038049f,  -0.036304f, -0.006922f, 0.000220f,
                -0.001677f, -0.027447f, -0.028009f, 0.049671f,  -0.028825f, -0.002372f, 0.014811f,  0.009737f,
                0.005833f,  0.013396f,  0.005499f,  0.037185f,  0.007348f,  0.040072f,  0.081456f,  0.071941f,
                -0.013184f, -0.042821f, -0.010932f, 0.004997f,  -0.009275f, 0.035000f,  -0.050986f, -0.000000f,
                -0.088554f, 0.023925f,  -0.016142f, 0.031668f,  0.027151f,  0.052448f,  -0.047114f, -0.058830f,
                -0.063226f, 0.040768f,  0.049837f,  0.106423f,  -0.074507f, -0.012395f, 0.018326f,  0.039482f,
                -0.024839f, 0.014511f,  -0.037098f, 0.020088f,  0.000104f,  0.009865f,  0.024824f,  -0.052554f,
                0.029343f,  -0.087162f, -0.014474f, 0.025960f,  -0.018732f, -0.076217f, 0.035081f,  0.103605f,
                -0.028070f, 0.012814f,  -0.076493f, -0.018743f, 0.024966f,  0.081539f,  0.068646f,  -0.064105f,
                -0.083820f, 0.061447f,  -0.033415f, -0.106160f, -0.040205f, 0.032567f,  0.076642f,  -0.072963f,
                0.000432f,  -0.040960f, -0.075837f, 0.027454f,  0.074693f,  0.017732f,  0.091085f,  0.110358f,
                0.000636f,  0.051421f,  -0.014600f, 0.033248f,  0.023641f,  -0.023067f, 0.038975f,  0.030210f};

            for (size_t i = 0; i < result.size(); ++i)
                REQUIRE_THAT(result[i], Catch::Matchers::WithinAbs(expected_embedding[i], 0.0001));
        }
    }

    SECTION("Embeddings are semantically meaningful") {
        std::string text_base = "The cat is sleeping on the couch.";
        std::string text_similar = "A kitten rests comfortably on the sofa.";
        std::string text_dissimilar = "The database cluster experienced severe latency resulting in downtime.";

        auto emb_base = model.embed(text_base);
        auto emb_similar = model.embed(text_similar);
        auto emb_dissimilar = model.embed(text_dissimilar);

        db.add(emb_base, 1);
        db.add(emb_similar, 2);
        db.add(emb_dissimilar, 3);

        auto results = db.search(emb_base, 3);

        REQUIRE(results.size() == 3);

        // Result should be closest to itself
        REQUIRE(results[0].id == 1);

        float dist_similar = 0;
        float dist_dissimilar = 0;

        for (auto const& res : results) {
            if (res.id == 2)
                dist_similar = res.distance;
            if (res.id == 3)
                dist_dissimilar = res.distance;
        }

        REQUIRE(dist_similar < dist_dissimilar);

        REQUIRE(results[1].id == 2);
        REQUIRE(results[2].id == 3);
    }
}
