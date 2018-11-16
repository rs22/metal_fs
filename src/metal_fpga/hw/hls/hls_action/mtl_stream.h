#ifndef __MTL_STREAM_H__
#define __MTL_STREAM_H__

#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_snap.H>

struct byte_stream_element {
    snapu8_t data;
    snap_bool_t last;
};
typedef hls::stream<byte_stream_element> mtl_byte_stream;

template<uint8_t NB>
struct stream_element {
    ap_uint<8 * NB> data;
    ap_uint<NB> keep;
    snap_bool_t last;
};

typedef stream_element<8> mtl_stream_element;
typedef hls::stream<mtl_stream_element> mtl_stream;

// // Convert a stream of bytes (uint8_t) to a stream of arbitrary width
// // (on byte boundaries) words
// template<typename T, bool BS>
// void stream_bytes2words(hls::stream<T> &words_out, mtl_byte_stream &bytes_in, snap_bool_t enable)
// {
//     if (enable) {
//         T tmpword;
//         byte_stream_element tmpbyte;
//         snap_bool_t last_byte = false;
//         BYTES2WORDS_LOOP:
//         for (int i = 0; last_byte == false; i++) {
//         //#pragma HLS loop_tripcount max=1488
//             bytes_in.read(tmpbyte);
//             last_byte = tmpbyte.last;
//             if (!BS) { // Shift bytes in little endian order
//                 tmpword.data = (tmpword.data >> 8) |
//                     (ap_uint<sizeof(tmpword.data) * 8>(tmpbyte.data) << ((sizeof(tmpword.data) - 1) * 8));
//                 tmpword.strb = (tmpword.strb >> 1) |
//                     (ap_uint<sizeof(tmpword.data)>(1) << (sizeof(tmpword.data) - 1) * 8);
//             } else { // Shift bytes in big endian order
//                 tmpword.data = (tmpword.data << 8) | ap_uint<sizeof(tmpword.data) * 8>(tmpbyte.data);
//                 tmpword.strb = (tmpword.strb << 1) | ap_uint<sizeof(tmpword.data)>(1);
//             }
//             if (i % sizeof(tmpword.data) == sizeof(tmpword.data) - 1 || last_byte) {
//                 tmpword.last = last_byte;
//                 words_out.write(tmpword);
//             }
//         }
//     }
// }

// // Convert an arbitrary width (on byte boundaries) words into a stream of
// // bytes (uint8_t)
// template<typename T, bool BS>
// void stream_words2bytes(mtl_byte_stream &bytes_out, hls::stream<T> &words_in, snap_bool_t enable)
// {
//     //if (enable) {
//         T inval;
//         ap_uint<sizeof(inval.data) * 8> tmpword;
//         ap_uint<sizeof(inval.data)> tmpstrb;
//         byte_stream_element outval;
//         WORDS2BYTES_LOOP:
//         while (enable) {
//             words_in.read(inval);
//             tmpword = inval.data;
//             tmpstrb = inval.strb;
//             for (int i = 0; i < sizeof(tmpword); i++) {
//                 if (!BS) { // shift bytes out in little endian order
//                     outval.data = uint8_t(tmpword);
//                     tmpword >>= 8;
//                     tmpstrb >>= 1;
//                     outval.last = inval.last && tmpstrb == ap_uint<sizeof(tmpword)>(0);
//                 } else { // shift bytes out in big endian order
//                     outval.data = uint8_t(tmpword >> ((sizeof(tmpword) - 1) * 8));
//                     tmpword <<= 8;
//                     tmpstrb <<= 1;
//                     outval.last = inval.last && tmpstrb == ap_uint<sizeof(tmpword)>(0);
//                 }
//                 bytes_out.write(outval);

//                 if (outval.last)
//                     break;
//             }

//             if (inval.last)
//                 break;
//         }
//     //}
// }

template<typename TOut, typename TIn>
void stream_widen(hls::stream<TOut> &words_out, hls::stream<TIn> &words_in, snap_bool_t enable)
{
    if (enable) {
        TIn inval;
        TOut tmpword;
        snap_bool_t last_word = false;
        for (uint64_t i = 0; last_word == false; i++) {
        //#pragma HLS loop_tripcount max=1488
            words_in.read(inval);
            last_word = inval.last;

            //tmpword = (tmpword >> 8) |
            // (ap_uint<NB * 8>(tmpbyte) << ((NB - 1) * 8));

            // Shift words in "big endian" order
            // tmpword.data = (tmpword.data << (sizeof(inval.data) * 8)) | ap_uint<sizeof(tmpword.data) * 8>(inval.data);
            // tmpword.strb = (tmpword.strb << sizeof(inval.data)) | ap_uint<sizeof(tmpword.data)>(inval.strb);

            // Shift words in "little endian" order
            tmpword.data = (tmpword.data >> (sizeof(inval.data) * 8)) | (ap_uint<sizeof(tmpword.data) * 8>(inval.data) << ((sizeof(tmpword.data) / sizeof(inval.data) - 1) * sizeof(inval.data) * 8));
            tmpword.strb = (tmpword.strb >> (sizeof(inval.strb))) | (ap_uint<sizeof(tmpword.strb)>(inval.strb) << ((sizeof(tmpword.data) / sizeof(inval.data) - 1) * sizeof(inval.data)));

            // tmpword.data = (tmpword.data << (sizeof(inval.data) * 8)) | ap_uint<sizeof(tmpword.data) * 8>(inval.data);
            // tmpword.strb = (tmpword.strb << sizeof(inval.data)) | ap_uint<sizeof(tmpword.data)>(inval.strb);

            if (last_word) {
                // If this was the last word, shift the rest and increase i so that the result is written afterwards
                tmpword.data >>= 8 * sizeof(inval.data) * ((sizeof(tmpword.data) / sizeof(inval.data) - 1) - (i % (sizeof(tmpword.data) / sizeof(inval.data))));
                tmpword.strb >>= sizeof(inval.data) * ((sizeof(tmpword.data) / sizeof(inval.data) - 1) - (i % (sizeof(tmpword.data) / sizeof(inval.data))));
                i += ((sizeof(tmpword.data) / sizeof(inval.data) - 1) - (i % (sizeof(tmpword.data) / sizeof(inval.data))));
            }

            if ((i + 1) % (sizeof(tmpword.data) / sizeof(inval.data)) == 0) {
                tmpword.last = last_word;
                words_out.write(tmpword);
            }
        }
    }
}

template<typename TOut, typename TIn>
void stream_narrow(hls::stream<TOut> &words_out, hls::stream<TIn> &words_in, snap_bool_t enable)
{
    if (enable) {
        TIn inval;
        ap_uint<sizeof(inval.data) * 8> tmpword;
        ap_uint<sizeof(inval.data)> tmpstrb;
        TOut outval;
        for (;;) {
            words_in.read(inval);
            tmpword = inval.data;
            tmpstrb = inval.strb;

            for (int i = 0; i < sizeof(tmpword) / sizeof(outval.data); i++) {

                // shift words out in "big endian" order
                // outval.data = ap_uint<sizeof(outval.data) * 8>(tmpword >> ((sizeof(tmpword) / sizeof(outval.data) - 1) * sizeof(outval.data) * 8));
                // outval.strb = ap_uint<sizeof(outval.data)>(tmpstrb >> ((sizeof(tmpword) / sizeof(outval.data) - 1) * sizeof(outval.data)));
                // tmpword <<= sizeof(outval.data) * 8;
                // tmpstrb <<= sizeof(outval.data);

                // shift words out in "little endian" order
                outval.data = ap_uint<sizeof(outval.data) * 8>(tmpword);
                outval.strb = ap_uint<sizeof(outval.data)>(tmpstrb);
                tmpword >>= sizeof(outval.data) * 8;
                tmpstrb >>= sizeof(outval.data);

                outval.last = inval.last && tmpstrb == ap_uint<sizeof(tmpword)>(0);

                words_out.write(outval);

                if (outval.last)
                    break;
            }

            if (inval.last)
                break;
        }
    }
}

#endif // __MTL_STREAM_H__
