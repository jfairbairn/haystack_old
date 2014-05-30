require 'net/http'

class Runner
  def initialize(cmd, wait_until_line_match=nil)
    io = IO.popen(cmd, 'r', :err => [:child, :out])
    @pid = io.pid
    if wait_until_line_match
      io.each_line do |line|
        puts line
        break if line =~ /^Bad bad/
      end
    end
    Thread.new do
      IO.copy_stream io, STDOUT
    end
  end

  def stop
    Process.kill "TERM", @pid
  end
end

describe 'haystack' do
  before :all do
    IO.popen(%w(make -j5), 'r') {|io| io.each_line{|l| print l}}
    fail('Make failed') unless $? == 0
    @runner = Runner.new(%w(./haystack -p 9999 -f hs_spec.hs), /^Bad bad/)
  end

  after :all do
    @runner.stop
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
