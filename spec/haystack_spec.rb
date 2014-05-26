require 'net/http'

describe 'haystack' do
  before :all do
    io = IO.popen('./haystack -p 9999 -f hs_spec.hs 2>&1')
    @pid = io.pid
    io.each_line do |line|
      puts line
      break if line =~ /^Bad bad/
    end
    Thread.new do
      io.each_line do |line|
        puts line
      end
    end
  end

  after :all do
    Process.kill "TERM", @pid
  end

  it 'accepts a file and serves it out again' do
    Net::HTTP.start('localhost', 9999) do |http|
      path = '/1234567890123456_12345678'
      req = Net::HTTP::Post.new(path)
      req['Content-Type'] = 'text/plain'
      req.body = "I am a n00b"
      res = http.request(req)

      req = Net::HTTP::Get.new(path)
      res = http.request(req)

      expect(res.code.to_i).to eq(200)
      expect(res['Content-Type']).to eq('text/plain')
      expect(res.body).to eq('I am a n00b')
    end

  end

end
