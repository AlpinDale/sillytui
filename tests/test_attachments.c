#include "llm/common.h"
#include "test_framework.h"
#include <string.h>

TEST(base64_encode_simple) {
  const unsigned char data[] = "Hello";
  char *encoded = base64_encode(data, 5);
  ASSERT_NOT_NULL(encoded);
  ASSERT_EQ_STR("SGVsbG8=", encoded);
  free(encoded);
  PASS();
}

TEST(base64_encode_empty) {
  const unsigned char data[] = "";
  char *encoded = base64_encode(data, 0);
  ASSERT_NOT_NULL(encoded);
  ASSERT_EQ_STR("", encoded);
  free(encoded);
  PASS();
}

TEST(base64_encode_binary) {
  const unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0xFF};
  char *encoded = base64_encode(data, 5);
  ASSERT_NOT_NULL(encoded);
  ASSERT_EQ_STR("AAECA/8=", encoded);
  free(encoded);
  PASS();
}

TEST(base64_encode_padding_one) {
  const unsigned char data[] = "ab";
  char *encoded = base64_encode(data, 2);
  ASSERT_NOT_NULL(encoded);
  ASSERT_EQ_STR("YWI=", encoded);
  free(encoded);
  PASS();
}

TEST(base64_encode_padding_two) {
  const unsigned char data[] = "a";
  char *encoded = base64_encode(data, 1);
  ASSERT_NOT_NULL(encoded);
  ASSERT_EQ_STR("YQ==", encoded);
  free(encoded);
  PASS();
}

TEST(base64_encode_longer) {
  const unsigned char data[] = "The quick brown fox jumps over the lazy dog";
  char *encoded = base64_encode(data, strlen((char *)data));
  ASSERT_NOT_NULL(encoded);
  ASSERT_EQ_STR("VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZw==",
                encoded);
  free(encoded);
  PASS();
}

TEST(get_file_extension_simple) {
  char *ext = get_file_extension("image.png");
  ASSERT_NOT_NULL(ext);
  ASSERT_EQ_STR("png", ext);
  free(ext);
  PASS();
}

TEST(get_file_extension_multiple_dots) {
  char *ext = get_file_extension("archive.tar.gz");
  ASSERT_NOT_NULL(ext);
  ASSERT_EQ_STR("gz", ext);
  free(ext);
  PASS();
}

TEST(get_file_extension_no_extension) {
  char *ext = get_file_extension("filename");
  ASSERT_NULL(ext);
  PASS();
}

TEST(get_file_extension_dot_only) {
  char *ext = get_file_extension(".");
  ASSERT_NULL(ext);
  PASS();
}

TEST(get_file_extension_dotfile) {
  char *ext = get_file_extension(".gitignore");
  ASSERT_NULL(ext);
  PASS();
}

TEST(get_file_extension_path) {
  char *ext = get_file_extension("/path/to/file.jpg");
  ASSERT_NOT_NULL(ext);
  ASSERT_EQ_STR("jpg", ext);
  free(ext);
  PASS();
}

TEST(get_file_extension_null) {
  char *ext = get_file_extension(NULL);
  ASSERT_NULL(ext);
  PASS();
}

TEST(get_image_mime_type_jpeg) {
  const char *mime = get_image_mime_type("jpg");
  ASSERT_NOT_NULL(mime);
  ASSERT_EQ_STR("image/jpeg", mime);
  PASS();
}

TEST(get_image_mime_type_jpeg_uppercase) {
  const char *mime = get_image_mime_type("JPEG");
  ASSERT_NOT_NULL(mime);
  ASSERT_EQ_STR("image/jpeg", mime);
  PASS();
}

TEST(get_image_mime_type_png) {
  const char *mime = get_image_mime_type("png");
  ASSERT_NOT_NULL(mime);
  ASSERT_EQ_STR("image/png", mime);
  PASS();
}

TEST(get_image_mime_type_gif) {
  const char *mime = get_image_mime_type("gif");
  ASSERT_NOT_NULL(mime);
  ASSERT_EQ_STR("image/gif", mime);
  PASS();
}

TEST(get_image_mime_type_webp) {
  const char *mime = get_image_mime_type("webp");
  ASSERT_NOT_NULL(mime);
  ASSERT_EQ_STR("image/webp", mime);
  PASS();
}

TEST(get_image_mime_type_invalid) {
  const char *mime = get_image_mime_type("txt");
  ASSERT_NULL(mime);
  PASS();
}

TEST(get_image_mime_type_null) {
  const char *mime = get_image_mime_type(NULL);
  ASSERT_NULL(mime);
  PASS();
}

TEST(is_image_file_jpeg) {
  ASSERT_TRUE(is_image_file("photo.jpg"));
  ASSERT_TRUE(is_image_file("photo.jpeg"));
  ASSERT_TRUE(is_image_file("photo.JPG"));
  ASSERT_TRUE(is_image_file("photo.JPEG"));
  PASS();
}

TEST(is_image_file_png) {
  ASSERT_TRUE(is_image_file("screenshot.png"));
  ASSERT_TRUE(is_image_file("screenshot.PNG"));
  PASS();
}

TEST(is_image_file_gif) {
  ASSERT_TRUE(is_image_file("animation.gif"));
  ASSERT_TRUE(is_image_file("animation.GIF"));
  PASS();
}

TEST(is_image_file_webp) {
  ASSERT_TRUE(is_image_file("modern.webp"));
  ASSERT_TRUE(is_image_file("modern.WEBP"));
  PASS();
}

TEST(is_image_file_not_image) {
  ASSERT_FALSE(is_image_file("document.txt"));
  ASSERT_FALSE(is_image_file("archive.zip"));
  ASSERT_FALSE(is_image_file("video.mp4"));
  ASSERT_FALSE(is_image_file("audio.mp3"));
  PASS();
}

TEST(is_image_file_no_extension) {
  ASSERT_FALSE(is_image_file("filename"));
  PASS();
}

TEST(is_image_file_with_path) {
  ASSERT_TRUE(is_image_file("/home/user/images/photo.png"));
  ASSERT_TRUE(is_image_file("../relative/path/image.jpg"));
  ASSERT_FALSE(is_image_file("/home/user/docs/file.txt"));
  PASS();
}

void run_attachment_tests(void) {
  TEST_SUITE("Image Attachment Utilities");

  RUN_TEST(base64_encode_simple);
  RUN_TEST(base64_encode_empty);
  RUN_TEST(base64_encode_binary);
  RUN_TEST(base64_encode_padding_one);
  RUN_TEST(base64_encode_padding_two);
  RUN_TEST(base64_encode_longer);

  RUN_TEST(get_file_extension_simple);
  RUN_TEST(get_file_extension_multiple_dots);
  RUN_TEST(get_file_extension_no_extension);
  RUN_TEST(get_file_extension_dot_only);
  RUN_TEST(get_file_extension_dotfile);
  RUN_TEST(get_file_extension_path);
  RUN_TEST(get_file_extension_null);

  RUN_TEST(get_image_mime_type_jpeg);
  RUN_TEST(get_image_mime_type_jpeg_uppercase);
  RUN_TEST(get_image_mime_type_png);
  RUN_TEST(get_image_mime_type_gif);
  RUN_TEST(get_image_mime_type_webp);
  RUN_TEST(get_image_mime_type_invalid);
  RUN_TEST(get_image_mime_type_null);

  RUN_TEST(is_image_file_jpeg);
  RUN_TEST(is_image_file_png);
  RUN_TEST(is_image_file_gif);
  RUN_TEST(is_image_file_webp);
  RUN_TEST(is_image_file_not_image);
  RUN_TEST(is_image_file_no_extension);
  RUN_TEST(is_image_file_with_path);
}
