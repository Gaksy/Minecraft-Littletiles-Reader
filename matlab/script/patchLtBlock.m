function patchLtBlock(lt_block, color, alpha)
    patch('Vertices', lt_block.getLtCubVertices(), 'Faces', getLtFaces(), 'FaceColor', color, 'FaceAlpha', alpha);
end